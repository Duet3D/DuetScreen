/*
 * XmlWidgets.cpp
 *
 * DuetScreen widgets made available to XML. Composite widgets that already exist in C++ (Button, Slider) stay native:
 * XML creates the C++ object, which is deleted again when LVGL deletes its root.
 *
 *   <ds_button text="…" translation_tag="…" bind_text="subject" icon="file.png" checkable="true"/>
 *   <ds_slider min="0" max="100" increment="1" decimals="0" label="…" label_tag="…"
 *              bind_label="subject" bind_value="subject" numberpad="true"/>
 *   <model_list model="om.fans" item="fan_card"/>
 *
 * Every widget also accepts the usual lv_obj attributes (width, flex_grow, style_*, hidden, …).
 */

#include "XmlWidgets.h"
#include "Debug.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/Slider.h"
#include "XmlActions.h"
#include "XmlListModel.h"
#include "XmlUi.h"
#include "helix_xml.h"
#include "i18n/i18n.h"
#include "src/xml/lv_xml_parser.h"
#include "src/xml/lv_xml_utils.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <memory>
#include <string>
#include <vector>

namespace UI::Xml
{
	namespace
	{
		const char* attrOr(const char** attrs, const char* name, const char* fallback)
		{
			const char* value = lv_xml_get_value_of(attrs, name);
			return value ? value : fallback;
		}

		lv_subject_t* findSubject(lv_xml_parser_state_t* state, const char** attrs, const char* attr)
		{
			const char* name = lv_xml_get_value_of(attrs, attr);
			if (name == nullptr || name[0] == '\0')
			{
				return nullptr;
			}
			lv_subject_t* subject = lv_xml_get_subject(&state->scope, name);
			if (subject == nullptr)
			{
				LOG_WARN("{:s}=\"{:s}\": no such subject", attr, name);
			}
			return subject;
		}

		/** Delete a heap-allocated C++ wrapper when LVGL deletes its root */
		template <typename T>
		void deleteWith(lv_obj_t* root, T* owner)
		{
			lv_obj_add_event_cb(
				root, [](lv_event_t* e) { delete static_cast<T*>(lv_event_get_user_data(e)); }, LV_EVENT_DELETE, owner);
		}

		/* ---------------------------------------------------------------- ds_button */

		class XmlButton : public Button
		{
		  public:
			XmlButton(const std::string& name, lv_obj_t* parent, std::string_view text)
				: Button(name, parent, text)
			{
			}

			std::string translationTag;
		};

		void* buttonCreate(lv_xml_parser_state_t* state, const char** attrs)
		{
			const char* tag = lv_xml_get_value_of(attrs, "translation_tag");
			const std::string text = tag ? _(tag) : attrOr(attrs, "text", "");

			auto* parent = static_cast<lv_obj_t*>(lv_xml_state_get_parent(state));
			auto* owner = new XmlButton(attrOr(attrs, "name", "button"), parent, text);
			if (tag)
			{
				owner->translationTag = tag;
			}

			lv_obj_t* root = owner->getRootPtr();
			deleteWith(root, owner);
			if (tag)
			{
				lv_obj_add_event_cb(
					root,
					[](lv_event_t* e)
					{
						auto* b = static_cast<XmlButton*>(lv_event_get_user_data(e));
						b->setText(_(b->translationTag));
					},
					LV_EVENT_TRANSLATION_LANGUAGE_CHANGED,
					owner);
			}
			return root;
		}

		void buttonApply(lv_xml_parser_state_t* state, const char** attrs)
		{
			lv_xml_obj_apply(state, attrs);

			auto* root = static_cast<lv_obj_t*>(lv_xml_state_get_item(state));
			auto* button = static_cast<Button*>(LvObj::fromPtr(root));

			for (int i = 0; attrs[i]; i += 2)
			{
				const char* name = attrs[i];
				const char* value = attrs[i + 1];
				if (lv_streq(name, "icon"))
				{
					button->setIcon(value);
				}
				else if (lv_streq(name, "checkable"))
				{
					button->setCheckable(lv_xml_to_bool(value));
				}
				else if (lv_streq(name, "text") && lv_xml_get_value_of(attrs, "translation_tag") == nullptr)
				{
					button->setText(value);
				}
			}

			if (lv_subject_t* subject = findSubject(state, attrs, "bind_text"))
			{
				lv_subject_add_observer_obj(
					subject,
					[](lv_observer_t* observer, lv_subject_t* s)
					{
						auto* b = static_cast<Button*>(lv_observer_get_user_data(observer));
						b->setText(lv_subject_get_string(s));
					},
					root,
					button);
			}
		}

		/* ---------------------------------------------------------------- ds_slider */

		class XmlSlider : public Slider
		{
		  public:
			XmlSlider(const std::string& name, lv_obj_t* parent)
				: Slider(name, parent)
			{
				setValueChangedCallback(
					[this](float value)
					{
						// A value written from a binding is not an edit
						if (updatingFromBinding)
						{
							return;
						}
						lv_obj_send_event(getRootPtr(), Actions::valueConfirmedEvent(), &value);
					});
			}

			/** Set the value without raising `value_confirmed` */
			void setValueQuietly(float value)
			{
				updatingFromBinding = true;
				setValue(value);
				updatingFromBinding = false;
			}

			void setRangeQuietly(float min, float max)
			{
				updatingFromBinding = true;
				setRange(min, max);
				updatingFromBinding = false;
			}

		  private:
			bool updatingFromBinding = false;
		};

		void* sliderCreate(lv_xml_parser_state_t* state, const char** attrs)
		{
			auto* parent = static_cast<lv_obj_t*>(lv_xml_state_get_parent(state));
			auto* owner = new XmlSlider(attrOr(attrs, "name", "slider"), parent);
			lv_obj_t* root = owner->getRootPtr();
			deleteWith(root, owner);
			return root;
		}

		void sliderApply(lv_xml_parser_state_t* state, const char** attrs)
		{
			lv_xml_obj_apply(state, attrs);

			auto* root = static_cast<lv_obj_t*>(lv_xml_state_get_item(state));
			auto& slider = static_cast<XmlSlider&>(*LvObj::fromPtr(root));

			const char* min = lv_xml_get_value_of(attrs, "min");
			const char* max = lv_xml_get_value_of(attrs, "max");
			if (min || max)
			{
				slider.setRangeQuietly(min ? static_cast<float>(lv_xml_atoi(min)) : slider.getMin(),
									   max ? static_cast<float>(lv_xml_atoi(max)) : slider.getMax());
			}
			if (const char* increment = lv_xml_get_value_of(attrs, "increment"))
			{
				slider.setIncrementValue(static_cast<float>(lv_xml_atoi(increment)));
			}
			if (const char* decimals = lv_xml_get_value_of(attrs, "decimals"))
			{
				slider.setDisplayDecimals(lv_xml_atoi(decimals));
			}
			if (const char* tag = lv_xml_get_value_of(attrs, "label_tag"))
			{
				slider.setLabel(_(tag));
			}
			else if (const char* label = lv_xml_get_value_of(attrs, "label"))
			{
				slider.setLabel(label);
			}
			if (lv_xml_to_bool(attrOr(attrs, "numberpad", "false")))
			{
				slider.setNumberPad(getNumberPad());
			}

			if (lv_subject_t* subject = findSubject(state, attrs, "bind_label"))
			{
				lv_subject_add_observer_obj(
					subject,
					[](lv_observer_t* observer, lv_subject_t* s)
					{ static_cast<Slider*>(lv_observer_get_user_data(observer))->setLabel(lv_subject_get_string(s)); },
					root,
					&slider);
			}

			if (lv_subject_t* subject = findSubject(state, attrs, "bind_value"))
			{
				lv_subject_add_observer_obj(
					subject,
					[](lv_observer_t* observer, lv_subject_t* s)
					{
						auto* o = static_cast<XmlSlider*>(lv_observer_get_user_data(observer));
						// Don't fight the user while they are typing a value
						if (o->isFocused())
						{
							return;
						}
						o->setValueQuietly(static_cast<float>(lv_subject_get_int(s)));
					},
					root,
					&slider);
			}
		}

		/* ---------------------------------------------------------------- model_list */

		class ModelListBinding : public ListModelBase::Listener
		{
		  public:
			ModelListBinding(lv_obj_t* list, ListModelBase& model, std::string item, lv_xml_subject_ctx_t* parentCtx)
				: m_list(list)
				, m_model(&model)
				, m_item(std::move(item))
				, m_parentCtx(lv_xml_subject_ctx_ref(parentCtx))
			{
				m_model->addListener(this);
				build(0, m_model->size());
			}

			~ModelListBinding() override
			{
				// Runs from the list's LV_EVENT_DELETE: LVGL deletes the item widgets right after, so only let go
				if (m_model)
				{
					m_model->removeListener(this);
				}
				m_entries.clear();
				lv_xml_subject_ctx_unref(m_parentCtx);
			}

			void onModelResized(size_t oldSize, size_t newSize) override
			{
				if (newSize > oldSize)
				{
					build(m_entries.size(), newSize);
				}
				else
				{
					truncate(newSize);
				}
			}

			void onModelDestroyed() override
			{
				truncate(0);
				m_model = nullptr;
			}

		  private:
			struct Entry
			{
				lv_obj_t* obj;
				std::unique_ptr<SubjectContext> ctx;
			};

			void build(size_t from, size_t to)
			{
				ZoneScoped;
				for (size_t i = from; i < to; i++)
				{
					auto ctx = std::make_unique<SubjectContext>(m_parentCtx);
					m_model->bindItem(i, *ctx);

					const std::string name = std::to_string(i);
					const char* attrs[] = {"name", name.c_str(), nullptr, nullptr};
					auto* obj =
						static_cast<lv_obj_t*>(lv_xml_create_with_ctx(m_list, m_item.c_str(), attrs, ctx->get()));
					if (obj == nullptr)
					{
						LOG_ERROR("<model_list model=\"{:s}\">: couldn't create item component '{:s}'",
								  m_model->getName(),
								  m_item);
					}
					else
					{
						Actions::registerScope(obj, nullptr, i);
					}
					m_entries.push_back({obj, std::move(ctx)});
				}
			}

			void truncate(size_t count)
			{
				while (m_entries.size() > count)
				{
					if (m_entries.back().obj)
					{
						lv_obj_delete(m_entries.back().obj);
					}
					m_entries.pop_back();
				}
			}

			lv_obj_t* m_list;
			ListModelBase* m_model;
			std::string m_item;
			lv_xml_subject_ctx_t* m_parentCtx;
			std::vector<Entry> m_entries;
		};

		void* listCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
		{
			return lv_obj_create(static_cast<lv_obj_t*>(lv_xml_state_get_parent(state)));
		}

		void listApply(lv_xml_parser_state_t* state, const char** attrs)
		{
			lv_xml_obj_apply(state, attrs);

			auto* list = static_cast<lv_obj_t*>(lv_xml_state_get_item(state));
			const char* modelName = lv_xml_get_value_of(attrs, "model");
			const char* item = lv_xml_get_value_of(attrs, "item");
			if (modelName == nullptr || item == nullptr)
			{
				LOG_WARN("<model_list> needs both model= and item=");
				return;
			}

			ListModelBase* model = ListModelBase::find(modelName);
			if (model == nullptr)
			{
				LOG_WARN("<model_list model=\"{:s}\">: no such model", modelName);
				return;
			}

			auto* binding = new ModelListBinding(list, *model, item, lv_xml_subject_ctx_get_active());
			deleteWith(list, binding);
		}
	} // namespace

	void registerWidgets()
	{
		lv_xml_register_widget("ds_button", buttonCreate, buttonApply);
		lv_xml_register_widget("ds_slider", sliderCreate, sliderApply);
		lv_xml_register_widget("model_list", listCreate, listApply);
	}
} // namespace UI::Xml
