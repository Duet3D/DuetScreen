#pragma once

#include "Model.h"
#include "UI/Components/Containers/Card.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "lvgl/src/osal/lv_os.h"
#include <fmt/ostream.h>
#include <memory>

namespace UI
{
	class BasePresenter;

	/**
	 * This is the base View, each screen should inherit from this class. It provides a link
	 * to the Presenter class.
	 *
	 * @tparam T The type of Presenter associated with this view.
	 *
	 * @note All views in the application must be a subclass of this type.
	 */
	template <class T, class BaseViewType = LvContainer>
		requires(std::is_base_of_v<BasePresenter, T> && std::is_base_of_v<LvObj, BaseViewType>)
	class View : public BaseViewType
	{
	  public:
		template <typename... Args>
		View(const std::string& name, LvObj* parent, Args&&... args)
			: BaseViewType(name, parent, std::forward<Args>(args)...)
			, m_presenter(std::make_shared<T>(static_cast<LvObj*>(this)))
		{
			m_presenter->init();
		}

		virtual ~View()
		{
			// Can't call deactivate here because any inherited classes will have been destroyed and if the presenter
			// uses `m_view` or `getView()` it will cause a crash since the memory for `m_view` will only contain data
			// from this base class. The compiler does not catch this.
			Model::get().unbind(m_presenter);
			if (m_presenter)
			{
				m_presenter->setView(nullptr);
			}
		}

		/**
		 * @brief Get a pointer to the MVP model
		 */
		Model& getModel() const { return m_presenter->getModel(); }

		std::shared_ptr<T> getPresenter() { return m_presenter; }

		void activate() { m_presenter->activate(); }
		void deactivate() { m_presenter->deactivate(); }

		/**
		 * @brief Shows the view by activating its presenter and then showing the view itself.
		 *
		 * @note This function calls the `onShow()` virtual method before showing the view.
		 */
		void show(bool move_to_front = false) final
		{
			activate();
			BaseViewType::show(move_to_front);
		}

		/**
		 * @brief Hides the view by deactivating its presenter and then hiding the view itself.
		 *
		 * @note This function calls the `onHide()` virtual method before hiding the view.
		 */
		void hide(bool move_to_back = false) final
		{
			deactivate();
			BaseViewType::hide(move_to_back);
		}

	  protected:
		std::shared_ptr<T> m_presenter;
	};

} // namespace UI
