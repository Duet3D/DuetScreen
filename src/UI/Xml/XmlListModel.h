/*
 * XmlListModel.h
 *
 * A list model is a named, dynamic-length list whose items each own a set of subjects. XML shows one with
 *
 *     <model_list model="om.fans" item="fan_card"/>
 *
 * which builds one `fan_card` per item. Each card is created with its own subject context, so inside `fan_card.xml`
 * `bind_text="name"` binds to *that* fan's name. Every item also has an `index` subject.
 *
 * Models notify the list widgets showing them when they grow or shrink; widgets add or remove items at the end, so
 * existing items (and their scroll position, focus and in-progress edits) survive a length change.
 */

#pragma once

#include "XmlSubjects.h"
#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	/** Adds an item's subjects to the context its widgets are built in */
	class ItemBinder
	{
	  public:
		explicit ItemBinder(SubjectContext& ctx)
			: m_ctx(ctx)
		{
		}

		template <typename S>
		ItemBinder& operator()(const std::string& name, S& subject)
		{
			m_ctx.add(name, subject);
			return *this;
		}

	  private:
		SubjectContext& m_ctx;
	};

	class ListModelBase
	{
	  public:
		class Listener
		{
		  public:
			virtual ~Listener() = default;
			/** Called after items are added, or before items are removed (so widgets can let go of them first) */
			virtual void onModelResized(size_t oldSize, size_t newSize) = 0;
			/** The model is going away; drop every widget bound to it */
			virtual void onModelDestroyed() = 0;
		};

		explicit ListModelBase(std::string name);
		virtual ~ListModelBase();

		ListModelBase(const ListModelBase&) = delete;
		ListModelBase& operator=(const ListModelBase&) = delete;

		const std::string& getName() const { return m_name; }

		virtual size_t size() const = 0;

		/** Add item `index`'s subjects (and its `index` subject) to `ctx` */
		virtual void bindItem(size_t index, SubjectContext& ctx) = 0;

		void addListener(Listener* listener);
		void removeListener(Listener* listener);

		/** Find a live model by name, or nullptr */
		static ListModelBase* find(std::string_view name);

	  protected:
		void notifyResized(size_t oldSize, size_t newSize);
		void notifyDestroyed();

	  private:
		std::string m_name;
		std::vector<Listener*> m_listeners;
	};

	struct ListItemBase
	{
		explicit ListItemBase(size_t i)
			: index(static_cast<int32_t>(i))
		{
		}
		virtual ~ListItemBase() = default;

		IntSubject index;
	};

	template <typename Item>
	concept ListItem = std::derived_from<Item, ListItemBase> && std::constructible_from<Item, size_t> &&
					   requires(Item& item, ItemBinder& binder) { item.bind(binder); };

	template <ListItem Item>
	class ListModel : public ListModelBase
	{
	  public:
		using ListModelBase::ListModelBase;

		~ListModel() override
		{
			// Widgets must let go of the items before the items' subjects are destroyed
			notifyDestroyed();
		}

		size_t size() const override { return m_items.size(); }

		Item& operator[](size_t index) { return *m_items.at(index); }
		const Item& operator[](size_t index) const { return *m_items.at(index); }

		void resize(size_t count)
		{
			resize(count, [](Item&, size_t) {});
		}

		/**
		 * Resize, calling `initNew(item, index)` for each added item *before* list widgets are told, so they are
		 * built showing real values rather than defaults that change a moment later.
		 */
		template <typename F>
			requires std::invocable<F, Item&, size_t>
		void resize(size_t count, F&& initNew)
		{
			const size_t oldSize = m_items.size();
			if (count == oldSize)
			{
				return;
			}

			if (count > oldSize)
			{
				m_items.reserve(count);
				for (size_t i = oldSize; i < count; i++)
				{
					m_items.push_back(std::make_unique<Item>(i));
					initNew(*m_items.back(), i);
				}
				notifyResized(oldSize, count);
			}
			else
			{
				notifyResized(oldSize, count);
				m_items.resize(count);
			}
		}

		void bindItem(size_t index, SubjectContext& ctx) override
		{
			Item& item = *m_items.at(index);
			ItemBinder binder(ctx);
			binder("index", item.index);
			item.bind(binder);
		}

	  private:
		// unique_ptr: subjects are bound by address and must never move
		std::vector<std::unique_ptr<Item>> m_items;
	};
} // namespace UI::Xml
