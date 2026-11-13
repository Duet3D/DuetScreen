"""
Review the images written by the screenshot tests, and act on the decisions.

A test that finds no reference image writes one, and a test whose screenshot no longer
matches its reference writes the new screenshot next to it with an `_err` suffix. Each
of those becomes a ReviewItem, and the reviewer decides per item:

  - a new reference ("new") is kept, or deleted so the next run writes it again;
  - a changed screenshot ("diff") replaces its reference, or the reference is kept.

Two front ends collect the decisions, and both leave an undecided item untouched:

  - review_in_browser() serves a page on 127.0.0.1 and opens it with the `webbrowser`
    module. It needs no display in the process running it: in a VS Code devcontainer
    `$BROWSER` opens the page on the host and forwards the port, so it works without
    X11 or Wayland access from the container.
  - review_in_terminal() asks about each item in the terminal.
"""

from __future__ import annotations

import io
import json
import secrets
import threading
import webbrowser
from dataclasses import dataclass, field
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Callable, Dict, List, Optional

PAGE_PATH = Path(__file__).with_name("image_review.html")  # The browser reviewer's page
ERR_SUFFIX = "_err.png"  # Suffix of a screenshot that did not match its reference

KIND_NEW = "new"  # A reference image the test run created
KIND_DIFF = "diff"  # A screenshot that differs from its reference

ACCEPT = "accept"  # Keep the new reference / replace the reference with the screenshot
REJECT = "reject"  # Delete the new reference / keep the old reference
DECISIONS = (ACCEPT, REJECT)  # Every decision an item can carry besides "undecided"

VIEWS = ("reference", "new", "diff", "overlay")  # Images the browser page can ask for


def reference_for(err_path: Path) -> Path:
	"""Return the reference image an `_err` screenshot was compared against."""
	return err_path.with_name(err_path.name[:-len(ERR_SUFFIX)] + ".png")


@dataclass
class ReviewItem:
	"""One image waiting for the reviewer's decision."""

	kind: str  # KIND_NEW or KIND_DIFF
	image: Path  # The image the test run wrote: the new reference, or the `_err` screenshot
	label: str  # Name shown to the reviewer, relative to its reference directory
	decision: Optional[str] = None  # ACCEPT, REJECT, or None while undecided

	@property
	def reference(self) -> Optional[Path]:
		"""The existing reference a changed screenshot replaces, or None for a new one."""
		return reference_for(self.image) if self.kind == KIND_DIFF else None


@dataclass
class ReviewSummary:
	"""What applying the decisions changed on disk."""

	updated: List[str] = field(default_factory=list)  # References replaced by their screenshot
	kept_old: List[str] = field(default_factory=list)  # References kept despite a difference
	kept_new: List[str] = field(default_factory=list)  # New references kept
	deleted_new: List[str] = field(default_factory=list)  # New references deleted
	undecided: List[str] = field(default_factory=list)  # Items left as the test run wrote them
	failed: List[str] = field(default_factory=list)  # Items whose change could not be applied
	abandoned: bool = False  # True when the review was cancelled before applying anything

	def log_to(self, log: Callable[[str], None]) -> None:
		"""Write a one-line-per-group account of the summary through `log`."""
		if self.abandoned:
			log("Review cancelled: no reference images were changed.")
			return
		groups = (
			("Updated reference", self.updated),
			("Kept old reference", self.kept_old),
			("Kept new reference", self.kept_new),
			("Deleted new reference", self.deleted_new),
			("Left undecided", self.undecided),
			("Failed", self.failed),
		)
		for title, names in groups:
			if names:
				log(f"  {title} ({len(names)}): {', '.join(names)}")

	def to_json(self) -> Dict[str, object]:
		"""Return the summary as a JSON-ready dict for the browser page."""
		return {
			"updated": self.updated,
			"keptOld": self.kept_old,
			"keptNew": self.kept_new,
			"deletedNew": self.deleted_new,
			"undecided": self.undecided,
			"failed": self.failed,
		}


def apply_decisions(items: List[ReviewItem]) -> ReviewSummary:
	"""Carry out each item's decision on disk and report what changed."""
	summary = ReviewSummary()
	for item in items:
		try:
			if item.decision is None:
				summary.undecided.append(item.label)
			elif item.kind == KIND_DIFF and item.decision == ACCEPT:
				item.reference.write_bytes(item.image.read_bytes())
				item.image.unlink()
				summary.updated.append(item.label)
			elif item.kind == KIND_DIFF:
				summary.kept_old.append(item.label)
			elif item.decision == ACCEPT:
				summary.kept_new.append(item.label)
			else:
				item.image.unlink()
				summary.deleted_new.append(item.label)
		except OSError as e:
			summary.failed.append(f"{item.label} ({e})")
	return summary


# ---------------------------------------------------------------------------------------
# Terminal review
# ---------------------------------------------------------------------------------------


def _ask(prompt: str) -> str:
	"""Ask a y/n/a/q question and return the answer; end of input counts as 'q'."""
	while True:
		try:
			answer = input(f"{prompt} [y/N/a/q]: ").strip().lower()
		except EOFError:
			return "q"
		if answer in ("", "y", "n", "a", "q"):
			return answer or "n"
		print("Please answer 'y', 'n', 'a' (yes to all remaining) or 'q' (stop asking).", flush=True)


def review_in_terminal(items: List[ReviewItem], log: Callable[[str], None]) -> ReviewSummary:
	"""Ask about each item in the terminal, then apply the answers.

	'a' accepts the item and every one after it. 'q' stops asking; the answers given so
	far are still applied and the rest are left undecided.
	"""
	accept_rest = False
	for item in items:
		if accept_rest:
			item.decision = ACCEPT
			continue
		log("")
		if item.kind == KIND_DIFF:
			log(f"Changed screenshot: {item.label}")
			log(f"  Reference:  {item.reference}")
			log(f"  New (_err): {item.image}")
			answer = _ask("Replace the reference with the new screenshot?")
		else:
			log(f"New reference image: {item.label}")
			log(f"  {item.image}")
			answer = _ask("Keep this new reference image?")
		if answer == "q":
			break
		accept_rest = answer == "a"
		item.decision = ACCEPT if answer in ("y", "a") else REJECT
	return apply_decisions(items)


# ---------------------------------------------------------------------------------------
# Browser review
# ---------------------------------------------------------------------------------------


@dataclass
class _ItemImages:
	"""The PNG bytes the page shows for one item, read before anything is changed."""

	new: bytes  # The image the test run wrote
	reference: Optional[bytes] = None  # The existing reference, for a changed screenshot
	diff: Optional[bytes] = None  # Per-pixel difference, contrast stretched
	overlay: Optional[bytes] = None  # The new screenshot in grey with differences in red
	stats: Dict[str, object] = field(default_factory=dict)  # Sizes and changed-pixel count


def _png_bytes(image) -> bytes:
	"""Encode a Pillow image as PNG."""
	out = io.BytesIO()
	image.save(out, format="PNG")
	return out.getvalue()


def _compare(reference: bytes, new: bytes) -> tuple[Optional[bytes], Optional[bytes], Dict[str, object]]:
	"""Build the difference and overlay images for a changed screenshot.

	Returns (diff, overlay, stats). Without Pillow there are no pictures and no stats,
	and the page shows only the reference and the new screenshot.
	"""
	try:
		from PIL import Image, ImageChops, ImageOps
	except ImportError:
		return None, None, {}

	ref = Image.open(io.BytesIO(reference)).convert("RGBA")
	err = Image.open(io.BytesIO(new)).convert("RGBA")
	stats: Dict[str, object] = {"referenceSize": list(ref.size), "newSize": list(err.size)}

	# Compare on a canvas big enough for both, so a size change shows up as a difference.
	size = (max(ref.width, err.width), max(ref.height, err.height))

	def padded(img):
		"""Place `img` at the top left of a transparent canvas of the shared size."""
		if img.size == size:
			return img
		canvas = Image.new("RGBA", size, (0, 0, 0, 0))
		canvas.paste(img, (0, 0))
		return canvas

	ref, err = padded(ref), padded(err)

	# The difference is taken on RGB only, so it isn't hidden by a transparent alpha.
	raw = ImageChops.difference(ref.convert("RGB"), err.convert("RGB"))
	mask = raw.convert("L")
	stats["changedPixels"] = mask.point(lambda v: 255 if v else 0).histogram()[255]
	diff = ImageOps.autocontrast(raw)

	red = Image.new("RGBA", size, (255, 0, 0, 0))
	red.putalpha(ImageOps.autocontrast(mask))
	overlay = Image.alpha_composite(ImageOps.grayscale(err).convert("RGBA"), red)

	return _png_bytes(diff), _png_bytes(overlay), stats


class _BrowserSession:
	"""The state the browser page reads and changes: items, their images, and the outcome."""

	def __init__(self, items: List[ReviewItem], log: Callable[[str], None]):
		"""Read every item's images up front, so the page can still show them after changes."""
		self.items = items  # Items in the order the page lists them
		self.token = secrets.token_urlsafe(16)  # Secret path prefix; other pages can't guess it
		self.lock = threading.Lock()  # Guards decisions and the summary across request threads
		self.summary: Optional[ReviewSummary] = None  # Set once the reviewer finishes
		self.finished = threading.Event()  # Set once the summary is ready
		self.images: List[_ItemImages] = []  # Image bytes for each item, by index

		for index, item in enumerate(items, start=1):
			log(f"  Preparing {index}/{len(items)}: {item.label}")
			images = _ItemImages(new=item.image.read_bytes())
			if item.kind == KIND_DIFF:
				images.reference = item.reference.read_bytes() if item.reference.exists() else None
				if images.reference is not None:
					images.diff, images.overlay, images.stats = _compare(images.reference, images.new)
			self.images.append(images)

	def items_json(self) -> Dict[str, object]:
		"""Describe every item and its views for the page."""
		with self.lock:
			listed = []
			for index, (item, images) in enumerate(zip(self.items, self.images)):
				listed.append({
					"kind": item.kind,
					"label": item.label,
					"decision": item.decision,
					"views": [view for view in VIEWS if getattr(images, view) is not None],
					"stats": images.stats,
				})
			return {
				"items": listed,
				"finished": self.summary.to_json() if self.summary else None,
			}

	def image(self, index: int, view: str) -> Optional[bytes]:
		"""Return the PNG for one view of one item, or None if there isn't one."""
		if not 0 <= index < len(self.images) or view not in VIEWS:
			return None
		return getattr(self.images[index], view)

	def decide(self, index: int, decision: Optional[str]) -> bool:
		"""Record (or clear, with None) an item's decision; False if the request is invalid."""
		with self.lock:
			if self.summary or not 0 <= index < len(self.items):
				return False
			if decision is not None and decision not in DECISIONS:
				return False
			self.items[index].decision = decision
			return True

	def finish(self) -> ReviewSummary:
		"""Apply the decisions once; later calls return the same summary."""
		with self.lock:
			if self.summary is None:
				self.summary = apply_decisions(self.items)
				self.finished.set()
			return self.summary


def _make_handler(session: _BrowserSession) -> type:
	"""Create the request handler class that serves `session`."""

	class Handler(BaseHTTPRequestHandler):
		"""Serves the page, its JSON API and its images under /<token>/."""

		def log_message(self, format: str, *args) -> None:
			"""Keep request logging out of the test output."""

		def _route(self) -> Optional[List[str]]:
			"""Split the path after the session token, or None if the token is wrong."""
			parts = self.path.split("?", 1)[0].strip("/").split("/")
			if not parts or parts[0] != session.token:
				return None
			return parts[1:]

		def _send(self, status: HTTPStatus, body: bytes, content_type: str, cache: bool = False) -> None:
			"""Write a complete response."""
			self.send_response(status)
			self.send_header("Content-Type", content_type)
			self.send_header("Content-Length", str(len(body)))
			self.send_header("Cache-Control", "private, max-age=3600" if cache else "no-store")
			self.end_headers()
			self.wfile.write(body)

		def _send_json(self, value: object, status: HTTPStatus = HTTPStatus.OK) -> None:
			"""Write `value` as a JSON response."""
			self._send(status, json.dumps(value).encode(), "application/json")

		def _not_found(self) -> None:
			"""Write a 404 response."""
			self._send(HTTPStatus.NOT_FOUND, b"Not found", "text/plain")

		def do_GET(self) -> None:
			"""Serve the page (`/`), the items (`/api/items`) or an image (`/img/<i>/<view>.png`)."""
			route = self._route()
			if route == [] and not self.path.split("?", 1)[0].endswith("/"):
				# The page's links are relative to /<token>/, so it must be loaded from there.
				self.send_response(HTTPStatus.MOVED_PERMANENTLY)
				self.send_header("Location", f"/{session.token}/")
				self.send_header("Content-Length", "0")
				self.end_headers()
			elif route == [""] or route == []:
				self._send(HTTPStatus.OK, PAGE_PATH.read_bytes(), "text/html; charset=utf-8")
			elif route == ["api", "items"]:
				self._send_json(session.items_json())
			elif route is not None and len(route) == 3 and route[0] == "img" and route[2].endswith(".png"):
				try:
					png = session.image(int(route[1]), route[2][:-len(".png")])
				except ValueError:
					png = None
				if png is None:
					self._not_found()
				else:
					# Images never change within a session, and the token makes URLs unique per session.
					self._send(HTTPStatus.OK, png, "image/png", cache=True)
			else:
				self._not_found()

		def do_POST(self) -> None:
			"""Record a decision (`/api/items/<i>/decision`) or finish (`/api/finish`)."""
			route = self._route()
			length = int(self.headers.get("Content-Length") or 0)
			try:
				body = json.loads(self.rfile.read(length) or b"{}")
			except json.JSONDecodeError:
				body = None
			if not isinstance(body, dict):
				self._send_json({"error": "Expected a JSON object"}, HTTPStatus.BAD_REQUEST)
			elif route is not None and len(route) == 4 and route[:2] == ["api", "items"] and route[3] == "decision":
				try:
					ok = session.decide(int(route[2]), body.get("decision"))
				except ValueError:
					ok = False
				if ok:
					self._send_json(session.items_json())
				else:
					self._send_json({"error": "Invalid decision"}, HTTPStatus.BAD_REQUEST)
			elif route == ["api", "finish"]:
				self._send_json(session.finish().to_json())
			else:
				self._not_found()

	return Handler


def review_in_browser(items: List[ReviewItem], log: Callable[[str], None], port: int = 0) -> ReviewSummary:
	"""Serve a review page on 127.0.0.1, open it in a browser and wait for the reviewer.

	Decisions are applied when the reviewer presses Finish on the page. Ctrl+C in the
	terminal cancels the review without changing anything. `port` 0 picks a free port.
	"""
	session = _BrowserSession(items, log)
	server = ThreadingHTTPServer(("127.0.0.1", port), _make_handler(session))
	server.daemon_threads = True
	url = f"http://127.0.0.1:{server.server_address[1]}/{session.token}/"
	thread = threading.Thread(target=server.serve_forever, daemon=True)
	thread.start()

	log(f"Review the images at {url}")
	if not webbrowser.open(url):
		log("  (No browser could be opened automatically; open the address above.)")
	log("  Press Finish on the page to apply your decisions, or Ctrl+C here to cancel.")

	try:
		# Waiting in short slices keeps the main thread responsive to Ctrl+C.
		while not session.finished.wait(0.5):
			pass
	except KeyboardInterrupt:
		return ReviewSummary(abandoned=True)
	finally:
		server.shutdown()
		server.server_close()
	return session.summary
