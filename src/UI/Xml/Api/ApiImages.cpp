/*
 * ApiImages.cpp
 */

#include "ApiImages.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "UI/Widgets/Layout/DsLayout.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Layout/PlacementScope.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/XmlView.h"
#include "XmlTree.h"
#include "helix_xml.h"
#include <algorithm>
#include <fmt/format.h>
#include <optional>
#include <set>

namespace UI::Xml::Api
{
	namespace
	{
		constexpr int32_t MARGIN = 8;			// Space kept around what is pictured
		constexpr int32_t EXAMPLE_WIDTH = 480;	// Width of the area an example is laid out in
		constexpr int32_t EXAMPLE_HEIGHT = 320; // Height of the area an example is laid out in
		constexpr int32_t SWATCH_WIDTH = 160;	// Width of a style's sample box
		constexpr int32_t SWATCH_HEIGHT = 48;	// Height of a style's sample box

		/** A sample of LVGL's own widgets, as XML can use them, to show how a theme draws them */
		constexpr const char* LVGL_WIDGETS = R"(<component>
	<view extends="lv_obj" width="100%" height="content" flex_flow="row_wrap" style_pad_all="0" style_pad_gap="8">
		<lv_button><lv_label text="Button"/></lv_button>
		<lv_button checked="true"><lv_label text="Checked"/></lv_button>
		<lv_button disabled="true"><lv_label text="Disabled"/></lv_button>
		<lv_checkbox text="Checkbox"/>
		<lv_checkbox text="Checked" checked="true"/>
		<lv_switch/>
		<lv_switch checked="true"/>
		<lv_slider value="40" width="200"/>
		<lv_bar value="60" width="200"/>
		<lv_arc value="70" width="100" height="100"/>
		<lv_dropdown options="One&#10;Two&#10;Three"/>
		<lv_roller options="One&#10;Two&#10;Three" visible_row_count="3"/>
		<lv_textarea text="Text area" one_line="true" width="200"/>
		<lv_buttonmatrix map="'One' 'Two' 'Three'" width="300" height="60"/>
		<lv_table column_count="2" row_count="2">
			<lv_table-cell row="0" column="0" value="Cell"/>
			<lv_table-cell row="0" column="1" value="Cell"/>
			<lv_table-cell row="1" column="0" value="Cell"/>
			<lv_table-cell row="1" column="1" value="Cell"/>
		</lv_table>
		<lv_keyboard width="100%" height="200"/>
	</view>
</component>)";

		/** `text` escaped for an XML attribute; line breaks kept as character references */
		std::string escapeAttribute(std::string_view text)
		{
			std::string result;
			for (const char c : text)
			{
				switch (c)
				{
				case '&':
					result += "&amp;";
					break;
				case '<':
					result += "&lt;";
					break;
				case '"':
					result += "&quot;";
					break;
				case '\n':
					result += "&#10;";
					break;
				default:
					result += c;
				}
			}
			return result;
		}

		/** `area` grown by `by` on every side */
		lv_area_t grown(lv_area_t area, int32_t by)
		{
			area.x1 -= by;
			area.y1 -= by;
			area.x2 += by;
			area.y2 += by;
			return area;
		}

		/** The area `obj` covers */
		lv_area_t areaOf(lv_obj_t* obj)
		{
			lv_area_t area;
			lv_obj_get_coords(obj, &area);
			return area;
		}

		/** The area `obj`'s shown children cover, or nullopt if it has none */
		std::optional<lv_area_t> childrenArea(lv_obj_t* obj)
		{
			std::optional<lv_area_t> area;
			for (uint32_t i = 0; i < lv_obj_get_child_count(obj); i++)
			{
				lv_obj_t* child = lv_obj_get_child(obj, static_cast<int32_t>(i));
				if (lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN))
				{
					continue;
				}
				const lv_area_t coords = areaOf(child);
				if (area)
				{
					area->x1 = std::min(area->x1, coords.x1);
					area->y1 = std::min(area->y1, coords.y1);
					area->x2 = std::max(area->x2, coords.x2);
					area->y2 = std::max(area->y2, coords.y2);
				}
				else
				{
					area = coords;
				}
			}
			return area;
		}

		/** Whether `obj` is `ancestor` or inside it */
		bool isInside(lv_obj_t* obj, const lv_obj_t* ancestor)
		{
			for (; obj != nullptr; obj = lv_obj_get_parent(obj))
			{
				if (obj == ancestor)
				{
					return true;
				}
			}
			return false;
		}

		/** Draws the pictures */
		class Renderer
		{
		  public:
			/** Draw on `host`; see render() for `settle` and `picture` */
			Renderer(const Reference& reference,
					 lv_obj_t* host,
					 const std::function<void()>& settle,
					 const std::function<void(const std::string& file)>& picture)
				: m_reference(reference)
				, m_host(host)
				, m_settle(settle)
				, m_picture(picture)
			{
				// Text as the screen shows it: home.xml gives the whole screen the theme's `text` style, which views
				// and widgets inherit rather than set themselves
				if (lv_xml_style_t* text = lv_xml_get_style_by_name(nullptr, "text"))
				{
					m_screenText = lv_xml_style_get_lv_style(text);
					lv_obj_add_style(m_host, m_screenText, LV_PART_MAIN);
				}
			}

			/** Draw every picture */
			Images run()
			{
				Themes::Theme* active = Themes::getCurrentTheme();
				m_images.theme = active ? std::string(active->getName()) : "";
				for (const auto& theme : m_reference.themes)
				{
					Themes::Theme* loaded = theme.abstract ? nullptr : Themes::getThemeByName(theme.name);
					if (loaded == nullptr)
					{
						continue;
					}
					loaded->setThemeActive();
					ThemePictures& pictures = m_images.themes[theme.name];
					const std::string dir = fmt::format("themes/{:s}", theme.name);
					pictures.colours = colours(dir + "/colours");
					pictures.fonts = fonts(dir + "/fonts");
					styles(dir + "/styles", pictures.styles);
					widgets(dir + "/widgets", pictures.widgets);
					pictures.lvglWidgets = example(LVGL_WIDGETS,
												   "api_lvgl_widgets",
												   dir + "/lvgl_widgets",
												   lv_display_get_horizontal_resolution(nullptr) - 2 * MARGIN,
												   lv_display_get_vertical_resolution(nullptr) - 2 * MARGIN);
					pictures.home = home(dir + "/home");
					otherMode(*loaded, dir, pictures);
				}
				if (active)
				{
					active->setThemeActive();
				}
				views();
				placeable();
				return std::move(m_images);
			}

		  private:
			/** Draw the screen and return its pixels in `area`, 4 bytes each (blue, green, red, unused) */
			static std::vector<uint8_t> pixels(lv_area_t area)
			{
				lv_obj_t* screen = lv_screen_active();
				lv_obj_update_layout(screen);
				lv_obj_invalidate(screen);
				lv_refr_now(nullptr);

				std::vector<uint8_t> result;
				const lv_draw_buf_t* buffer = lv_display_get_buf_active(nullptr);
				if (buffer == nullptr ||
					lv_color_format_get_size(static_cast<lv_color_format_t>(buffer->header.cf)) != 4)
				{
					LOG_WARN("Pictures need a 32-bit display buffer holding the whole screen");
					return result;
				}
				for (int32_t y = std::max<int32_t>(area.y1, 0);
					 y <= std::min<int32_t>(area.y2, static_cast<int32_t>(buffer->header.h) - 1);
					 y++)
				{
					const uint8_t* row = buffer->data + static_cast<size_t>(y) * buffer->header.stride;
					for (int32_t x = std::max<int32_t>(area.x1, 0);
						 x <= std::min<int32_t>(area.x2, static_cast<int32_t>(buffer->header.w) - 1);
						 x++)
					{
						const uint8_t* pixel = row + static_cast<size_t>(x) * 4;
						result.insert(result.end(), {pixel[0], pixel[1], pixel[2], 0});
					}
				}
				return result;
			}

			/** Whether `area` of the screen is all one colour, so a picture of it would show nothing */
			static bool isBlank(const lv_area_t& area)
			{
				const auto drawn = pixels(area);
				for (size_t i = 4; i < drawn.size(); i += 4)
				{
					if (!std::equal(drawn.begin() + static_cast<ptrdiff_t>(i),
									drawn.begin() + static_cast<ptrdiff_t>(i + 4),
									drawn.begin()))
					{
						return false;
					}
				}
				return true;
			}

			/**
			 * Hand `area` of the screen to the caller as picture `name`, unless it shows nothing
			 * @return the picture's file, or empty if there is none
			 */
			std::string picture(lv_area_t area, std::string_view name)
			{
				const int32_t width = lv_display_get_horizontal_resolution(nullptr);
				const int32_t height = lv_display_get_vertical_resolution(nullptr);
				area.x1 = std::max<int32_t>(area.x1, 0);
				area.y1 = std::max<int32_t>(area.y1, 0);
				area.x2 = std::min<int32_t>(area.x2, width - 1);
				area.y2 = std::min<int32_t>(area.y2, height - 1);
				if (area.x2 < area.x1 || area.y2 < area.y1 || isBlank(area))
				{
					return "";
				}

				// Move what was drawn so the area is at the top left, and cut the display down to it
				const std::string file = fmt::format("{:s}/{:s}.png", IMAGES_DIR, name);
				lv_obj_set_pos(m_host, -area.x1, -area.y1);
				lv_display_set_resolution(nullptr, lv_area_get_width(&area), lv_area_get_height(&area));
				lv_obj_update_layout(lv_screen_active());
				m_picture(file);
				lv_display_set_resolution(nullptr, width, height);
				lv_obj_set_pos(m_host, 0, 0);
				lv_obj_update_layout(lv_screen_active());
				m_images.files.push_back(file);
				return file;
			}

			/** An empty container in the host, `width` × `height` at the margin */
			lv_obj_t* frame(int32_t width, int32_t height)
			{
				lv_obj_t* obj = lv_obj_create(m_host);
				lv_obj_remove_style_all(obj);
				lv_obj_set_pos(obj, MARGIN, MARGIN);
				lv_obj_set_size(obj, width, height);
				return obj;
			}

			/** Every palette colour, named */
			std::string colours(std::string_view name)
			{
				lv_obj_t* grid = frame(4 * (SWATCH_WIDTH + MARGIN), LV_SIZE_CONTENT);
				lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
				lv_obj_set_style_pad_gap(grid, MARGIN, LV_PART_MAIN);
				for (const auto& colourName : m_reference.colours)
				{
					const char* value = lv_xml_get_const_silent(nullptr, colourName.c_str());
					if (value == nullptr)
					{
						continue;
					}
					const lv_color_t colour = lv_xml_to_color(value);
					lv_obj_t* swatch = lv_obj_create(grid);
					lv_obj_remove_style_all(swatch);
					lv_obj_set_size(swatch, SWATCH_WIDTH, SWATCH_HEIGHT);
					lv_obj_set_style_bg_color(swatch, colour, LV_PART_MAIN);
					lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, LV_PART_MAIN);
					lv_obj_t* label = lv_label_create(swatch);
					lv_label_set_text(label, ("#" + colourName).c_str());
					lv_obj_set_style_text_color(
						label, lv_color_luminance(colour) > 128 ? lv_color_black() : lv_color_white(), LV_PART_MAIN);
					lv_obj_center(label);
				}
				lv_obj_update_layout(lv_screen_active());
				const std::string file = picture(grown(areaOf(grid), MARGIN), name);
				lv_obj_delete(grid);
				return file;
			}

			/** A line in each font, named, as wide as the screen allows: the largest fonts wrap */
			std::string fonts(std::string_view name)
			{
				lv_obj_t* list = frame(lv_display_get_horizontal_resolution(nullptr) - 2 * MARGIN, LV_SIZE_CONTENT);
				lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
				lv_obj_set_style_pad_gap(list, MARGIN, LV_PART_MAIN);
				for (const auto& fontName : m_reference.fonts)
				{
					const lv_font_t* font = lv_xml_get_font(nullptr, fontName.c_str());
					if (font == nullptr)
					{
						continue;
					}
					lv_obj_t* label = lv_label_create(list);
					lv_label_set_text(
						label,
						fmt::format("{:s}: The quick brown fox jumps over the lazy dog 0123456789", fontName).c_str());
					lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
					lv_obj_set_width(label, LV_PCT(100));
				}
				lv_obj_update_layout(lv_screen_active());
				const std::string file = picture(grown(areaOf(list), MARGIN), name);
				lv_obj_delete(list);
				return file;
			}

			/** Each style on a sample box, kept if it changes how the box looks */
			void styles(const std::string& dir, std::map<std::string, std::string>& index)
			{
				lv_obj_t* box = frame(SWATCH_WIDTH, SWATCH_HEIGHT);
				lv_obj_t* label = lv_label_create(box);
				lv_label_set_text(label, "Text");
				lv_obj_center(label);

				lv_obj_update_layout(lv_screen_active());
				const lv_area_t area = grown(areaOf(box), MARGIN);
				// Whether a style changes the box is judged without the screen's text style, which would hide what
				// the text styles do; the picture shows the box as the screen would
				setScreenText(false);
				const auto plain = pixels(area);
				Themes::forEachStyle(
					[&](std::string_view styleName, Themes::Style& style)
					{
						lv_obj_add_style(box, static_cast<lv_style_t*>(style), LV_PART_MAIN);
						const bool changes = pixels(area) != plain; // Else it changes nothing on its own
						setScreenText(true);
						if (changes)
						{
							const std::string file = picture(area, fmt::format("{:s}/{:s}", dir, styleName));
							if (!file.empty())
							{
								index[std::string(styleName)] = file;
							}
						}
						setScreenText(false);
						lv_obj_remove_style(box, static_cast<lv_style_t*>(style), LV_PART_MAIN);
					});
				setScreenText(true);
				lv_obj_delete(box);
			}

			/** Give what is pictured the screen's text style, or take it away */
			void setScreenText(bool on)
			{
				if (m_screenText == nullptr)
				{
					return;
				}
				lv_obj_remove_style(m_host, m_screenText, LV_PART_MAIN);
				if (on)
				{
					lv_obj_add_style(m_host, m_screenText, LV_PART_MAIN);
				}
				lv_obj_update_layout(lv_screen_active());
			}

			/** Each widget's example, in a component declaring the subjects it binds to */
			void widgets(const std::string& dir, std::map<std::string, std::string>& index)
			{
				for (const auto& widget : m_reference.widgets)
				{
					if (widget.example.empty())
					{
						continue;
					}
					std::string subjects;
					for (const auto& subject : widget.exampleSubjects)
					{
						subjects += fmt::format("\t\t<subject name=\"{:s}\" type=\"{:s}\" value=\"{:s}\"/>\n",
												escapeAttribute(subject.name),
												escapeAttribute(subject.type),
												escapeAttribute(subject.value));
					}
					const std::string xml =
						fmt::format("<component>\n\t<subjects>\n{:s}\t</subjects>\n"
									"\t<view extends=\"lv_obj\" width=\"100%\" height=\"100%\" flex_flow=\"column\" "
									"style_pad_all=\"0\">\n{:s}\n\t</view>\n</component>",
									subjects,
									widget.example);
					std::string component = "api_example_" + widget.name;
					std::replace(component.begin(), component.end(), '-', '_');
					const int32_t height = widget.exampleHeight > 0 ? widget.exampleHeight : EXAMPLE_HEIGHT;
					const std::string file =
						example(xml, component, fmt::format("{:s}/{:s}", dir, widget.name), EXAMPLE_WIDTH, height);
					if (!file.empty())
					{
						index[widget.name] = file;
					}
				}
			}

			/**
			 * Picture `xml`, a component registered for the purpose as `component`, laid out in an area `width` ×
			 * `height` (what overflows it isn't drawn) and cropped to what it draws
			 * @return the picture's file, or empty if there is none
			 */
			std::string example(const std::string& xml,
								const std::string& component,
								std::string_view name,
								int32_t width = EXAMPLE_WIDTH,
								int32_t height = EXAMPLE_HEIGHT)
			{
				if (lv_xml_register_component_from_data(component.c_str(), xml.c_str()) != LV_RESULT_OK)
				{
					LOG_WARN("{:s} doesn't load", component);
					return "";
				}
				lv_obj_t* area = frame(width, height);
				auto* view = static_cast<lv_obj_t*>(lv_xml_create(area, component.c_str(), nullptr));
				m_settle();
				std::string file;
				if (view != nullptr)
				{
					lv_obj_update_layout(lv_screen_active());
					if (auto drawn = childrenArea(view))
					{
						file = picture(grown(*drawn, MARGIN), name);
					}
				}
				// Before its component goes: an instance outliving its component frees what it shares
				lv_obj_delete(area);
				lv_xml_component_unregister(component.c_str());
				return file;
			}

			/**
			 * For a theme whose palette the user can change: the home screen and LVGL's widgets in the mode it isn't
			 * in, light or dark, which the user can switch it to. The palette the user chose is put back after.
			 */
			void otherMode(Themes::Theme& theme, const std::string& dir, ThemePictures& pictures)
			{
				if (!theme.isCustomisable())
				{
					return;
				}
				const Themes::PaletteInputs chosen = theme.getPalette();
				Themes::PaletteInputs other = chosen;
				other.dark = !chosen.dark;
				pictures.otherMode = other.dark ? "dark" : "light";
				theme.setPalette(other);
				theme.setThemeActive();
				pictures.homeOtherMode = home(fmt::format("{:s}/home_{:s}", dir, pictures.otherMode));
				pictures.lvglWidgetsOtherMode = example(LVGL_WIDGETS,
														"api_lvgl_widgets",
														fmt::format("{:s}/lvgl_widgets_{:s}", dir, pictures.otherMode),
														lv_display_get_horizontal_resolution(nullptr) - 2 * MARGIN,
														lv_display_get_vertical_resolution(nullptr) - 2 * MARGIN);
				theme.setPalette(chosen);
				theme.setThemeActive();
			}

			/** The home screen, as nothing contains it */
			std::string home(std::string_view name)
			{
				lv_obj_t* screenFrame = frame(LV_PCT(100), LV_PCT(100));
				lv_obj_set_pos(screenFrame, 0, 0);
				std::string file;
				{
					XmlView view("api_view", screenFrame, "home");
					view.show();
					m_settle();
					lv_obj_update_layout(lv_screen_active());
					file = picture(areaOf(view.getRootPtr()), name);
				}
				lv_obj_delete(screenFrame);
				return file;
			}

			/**
			 * Each component that is a view of its own, pictured unless it shows in another view's picture. Each view
			 * is drawn once to find what shows in it, then those to picture are drawn again for their pictures.
			 */
			void views()
			{
				std::vector<ViewNote> notes;
				for (const auto& component : m_reference.components)
				{
					if (component.isView())
					{
						drawView(component, [&](lv_obj_t* root) { notes.push_back(noteOf(component.name, root)); });
					}
				}

				for (const auto& note : notes)
				{
					const ViewNote* shownIn = nullptr;
					for (const auto& other : notes)
					{
						if (!other.blank && other.component != note.component && other.shows.contains(note.component))
						{
							shownIn = &other;
							break;
						}
					}
					if (note.blank || shownIn != nullptr)
					{
						continue;
					}
					const Component& component = componentNamed(note.component);
					drawView(component,
							 [&](lv_obj_t* root)
							 {
								 const std::string file = picture(areaOf(root), "views/" + component.name);
								 if (!file.empty())
								 {
									 m_images.views[component.name] = file;
									 m_images.components[component.name] = file;
								 }
							 });
				}

				// The others are seen in the picture of a view that shows them
				for (const auto& note : notes)
				{
					for (const auto& [owner, file] : m_images.views)
					{
						auto other = std::find_if(
							notes.begin(), notes.end(), [&](const ViewNote& n) { return n.component == owner; });
						if (!m_images.components.contains(note.component) && other != notes.end() &&
							other->shows.contains(note.component))
						{
							m_images.components[note.component] = file;
						}
					}
				}
			}

			/** The size a placed widget has in a slot: its size in pixels in a free layout, its own in a strip */
			struct PlacedSize
			{
				int32_t width = LV_SIZE_CONTENT;  // In pixels, or LV_SIZE_CONTENT
				int32_t height = LV_SIZE_CONTENT; // In pixels, or LV_SIZE_CONTENT
			};

			/**
			 * How big `widget` is placed in its first slot, as the home screen lays that slot out: `size` in a free
			 * layout, its own size in a strip
			 */
			static PlacedSize placedSize(const WidgetDef& widget, Size size)
			{
				const std::string& slot = widget.slots.front();
				const DsLayout* live = DsLayout::find(slot);
				const Layout* layout = Layouts::find(slot);
				if (live == nullptr || layout == nullptr)
				{
					return {}; // Not on the home screen: its own size
				}
				if (layout->kind == LayoutKind::Strip)
				{
					// A column (the sidebar) gives its widgets its width; a row (the status bar) its height
					if (lv_obj_get_style_flex_flow(live->getContainer(), LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN)
					{
						return {.width = lv_obj_get_content_width(live->getContainer())};
					}
					return {};
				}
				return {.width = std::min(size.width, layout->width), .height = std::min(size.height, layout->height)};
			}

			/**
			 * The settings `widget` is pictured with: its defaults, but a row setting (a fan, a tool) on the first row
			 * of its list, as the user would choose one that the printer has
			 */
			static nlohmann::json sampleSettings(const WidgetDef& widget)
			{
				nlohmann::json settings = nlohmann::json::object();
				for (const auto& setting : widget.settings)
				{
					if (setting.kind != SettingKind::Row)
					{
						continue;
					}
					if (const auto rows = setting.choices(); !rows.empty())
					{
						settings[setting.name] = rows.front().value;
					}
				}
				return settings;
			}

			/**
			 * Each widget a user can place, as placed: see the file comment of ApiImages.h. A free layout's widget is
			 * pictured at its smallest size too, to show how it fits what it shows to its size.
			 */
			void placeable()
			{
				// The home screen, to measure the slots
				lv_obj_t* screenFrame = frame(LV_PCT(100), LV_PCT(100));
				lv_obj_set_pos(screenFrame, 0, 0);
				std::map<std::string, PlacedSize> sizes;
				std::map<std::string, PlacedSize> smallest;
				{
					XmlView home("api_view", screenFrame, "home");
					home.show();
					m_settle();
					lv_obj_update_layout(lv_screen_active());
					for (const auto& widget : m_reference.placeable)
					{
						sizes[widget.id] = placedSize(widget, widget.defaultSize);
						if (sizes[widget.id].height != LV_SIZE_CONTENT && widget.minSize != widget.defaultSize)
						{
							smallest[widget.id] = placedSize(widget, widget.minSize);
						}
					}
				}
				lv_obj_delete(screenFrame);

				for (const auto& widget : m_reference.placeable)
				{
					const std::string file = drawPlaced(widget, sizes[widget.id], "widgets/" + widget.id);
					if (!file.empty())
					{
						m_images.placeable[widget.id] = file;
					}
					if (auto it = smallest.find(widget.id); it != smallest.end())
					{
						const std::string small = drawPlaced(widget, it->second, "widgets/" + widget.id + "_smallest");
						if (!small.empty())
						{
							m_images.placeableSmallest[widget.id] = small;
						}
					}
				}
			}

			/** Picture `widget` placed at `size` as `name`; @return the picture's file, or empty if none was kept */
			std::string drawPlaced(const WidgetDef& widget, PlacedSize size, const std::string& name)
			{
				lv_obj_t* area = frame(size.width, size.height);
				std::string file;
				{
					// A free widget's place is its area, as <ds_layout> measures its box; a strip's isn't measured
					PlacementScope place;
					if (size.height != LV_SIZE_CONTENT)
					{
						place.follow(area);
					}
					XmlView view(widget.id,
								 area,
								 widget.component,
								 nullptr,
								 widget.propsFor(sampleSettings(widget)),
								 &place.getScope());
					// A free widget fills its box, as <ds_layout> makes it
					if (size.height != LV_SIZE_CONTENT)
					{
						lv_obj_set_size(view.getRootPtr(), LV_PCT(100), LV_PCT(100));
					}
					view.show();
					m_settle();
					lv_obj_update_layout(lv_screen_active());
					file = picture(grown(areaOf(area), MARGIN), name);
				}
				lv_obj_delete(area);
				return file;
			}

			/**
			 * Build view `component` full screen, let it settle and call `drawn` with its root, then remove it. One a
			 * `<ds_view>` shows is built through a copy of that `<ds_view>`, so it has the size and presenter it is
			 * given there; one shown from C++ is built on its own.
			 */
			void drawView(const Component& component, const std::function<void(lv_obj_t* root)>& drawn)
			{
				lv_obj_t* screenFrame = frame(LV_PCT(100), LV_PCT(100));
				lv_obj_set_pos(screenFrame, 0, 0);
				if (auto placement = placementOf(component))
				{
					drawPlaced(component.name, *placement, screenFrame, drawn);
				}
				else
				{
					XmlView view("api_view", screenFrame, component.name);
					view.show();
					m_settle();
					lv_obj_update_layout(lv_screen_active());
					drawn(view.getRootPtr());
				}
				lv_obj_delete(screenFrame);
			}

			/** The component called `name`, which must exist */
			const Component& componentNamed(const std::string& name) const
			{
				return *std::find_if(m_reference.components.begin(),
									 m_reference.components.end(),
									 [&](const Component& c) { return c.name == name; });
			}

			/** The first `<ds_view>` that shows `component`, as written; nullopt if none does */
			std::optional<Element> placementOf(const Component& component) const
			{
				for (const auto& use : component.uses)
				{
					if (use.as != Use::As::View)
					{
						continue;
					}
					auto user = std::find_if(m_reference.components.begin(),
											 m_reference.components.end(),
											 [&](const Component& c) { return c.name == use.by; });
					std::string error;
					const auto root =
						user == m_reference.components.end() ? std::nullopt : parseXmlFile(user->source, error);
					if (const Element* placement = root ? findView(*root, component.name) : nullptr)
					{
						return *placement;
					}
				}
				return std::nullopt;
			}

			/** The first `<ds_view view="name">` in `element`, or nullptr */
			static const Element* findView(const Element& element, const std::string& name)
			{
				const std::string* view = element.attr("view");
				if (element.tag == "ds_view" && view && *view == name)
				{
					return &element;
				}
				for (const auto& child : element.children)
				{
					if (const Element* found = findView(child, name))
					{
						return found;
					}
				}
				return nullptr;
			}

			/** Build component `name` through `placement`, a copy of the `<ds_view>` that shows it, and call `drawn` */
			void drawPlaced(const std::string& name,
							const Element& placement,
							lv_obj_t* screenFrame,
							const std::function<void(lv_obj_t* root)>& drawn)
			{
				std::string attributes;
				for (const auto& [attr, value] : placement.attrs)
				{
					if (attr != "shown")
					{
						attributes += fmt::format(" {:s}=\"{:s}\"", attr, escapeAttribute(value));
					}
				}
				const std::string xml = fmt::format("<component>\n\t<view extends=\"lv_obj\" width=\"100%\" "
													"height=\"100%\" flex_flow=\"column\" style_pad_all=\"0\">\n"
													"\t\t<ds_view{:s} shown=\"true\"/>\n\t</view>\n</component>",
													attributes);
				const std::string wrapper = "api_view_" + name;
				if (lv_xml_register_component_from_data(wrapper.c_str(), xml.c_str()) != LV_RESULT_OK)
				{
					LOG_WARN("The <ds_view> showing {:s} doesn't load", name);
					return;
				}
				auto* view = static_cast<lv_obj_t*>(lv_xml_create(screenFrame, wrapper.c_str(), nullptr));
				m_settle();
				if (view != nullptr && lv_obj_get_child_count(view) > 0)
				{
					lv_obj_update_layout(lv_screen_active());
					drawn(lv_obj_get_child(view, 0));
				}
				// Before its component goes: an instance outliving its component frees what it shares
				if (view != nullptr)
				{
					lv_obj_delete(view);
				}
				lv_xml_component_unregister(wrapper.c_str());
			}

			/** A view, drawn: what it showed */
			struct ViewNote
			{
				std::string component;		   // The view's component
				bool blank = false;			   // It showed nothing
				std::set<std::string> shows{}; // The other views that show in it
			};

			/** What view `component`, drawn as `root`, shows: anything, and which other views */
			static ViewNote noteOf(const std::string& component, lv_obj_t* root)
			{
				ViewNote note{.component = component, .blank = isBlank(areaOf(root)), .shows = {}};
				for (const XmlView* view : getViews())
				{
					lv_obj_t* obj = view->getRootPtr();
					if (view->getComponent() != component && isInside(obj, root) && lv_obj_is_visible(obj))
					{
						note.shows.insert(view->getComponent());
					}
				}
				return note;
			}

			const Reference& m_reference; // What to picture
			lv_obj_t* m_host;			  // Where pictures are drawn
			lv_style_t* m_screenText =
				nullptr; // The theme's `text` style, given to the host as home.xml gives the screen
			const std::function<void()>& m_settle;						   // Lets Model events reach a view
			const std::function<void(const std::string& file)>& m_picture; // Compares a picture
			Images m_images;											   // The pictures so far
		};
	} // namespace

	Images render(const Reference& reference,
				  const std::function<void()>& settle,
				  const std::function<void(const std::string& file)>& picture)
	{
		// The host is the screen's size, fixed, so what is drawn keeps its layout when the display is cut down
		lv_obj_t* host = lv_obj_create(lv_screen_active());
		lv_obj_remove_style_all(host);
		lv_obj_set_size(
			host, lv_display_get_horizontal_resolution(nullptr), lv_display_get_vertical_resolution(nullptr));
		lv_obj_add_flag(host, LV_OBJ_FLAG_FLOATING);
		Images images = Renderer(reference, host, settle, picture).run();
		lv_obj_delete(host);
		return images;
	}
} // namespace UI::Xml::Api
