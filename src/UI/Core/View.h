#pragma once

#include "Model.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "lvgl/src/osal/lv_os.h"
#include <fmt/ostream.h>
#include <memory>

namespace UI
{
	class BasePresenter;

	/**
	 * This is the base View. It is used to create a dynamic component that has its own `Presenter`.
	 *
	 * The `Presenter` is responsible for providing data and logic for the component. It is created automatically when
	 * the `View` is constructed and destroyed when the `View` is destroyed.
	 *
	 * @tparam Presenter The type of Presenter associated with this view, must be inherited from `BasePresenter`.
	 * @tparam BaseViewType The type of BaseView associated with this view, must be inherited from `LvObj`.
	 */
	template <class Presenter, class BaseViewType = LvContainer>
		requires(std::is_base_of_v<BasePresenter, Presenter> && std::is_base_of_v<LvObj, BaseViewType>)
	class View : public BaseViewType
	{
	  public:
		template <typename... Args>
			requires(std::is_constructible_v<BaseViewType, Args...>)
		View(Args&&... args)
			: BaseViewType(std::forward<Args>(args)...)
			, m_presenter(std::make_shared<Presenter>(static_cast<LvObj*>(this)))
		{
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

		std::shared_ptr<Presenter>& getPresenter() { return m_presenter; }

		void activate() { m_presenter->activate(); }
		void deactivate() { m_presenter->deactivate(); }

		/**
		 * @brief Shows the view by activating its presenter and then showing the view itself.
		 *
		 * @note This function calls the `onShow()` virtual method before showing the view.
		 */
		void showInner() override
		{
			activate();
			BaseViewType::showInner();
		}

		/**
		 * @brief Hides the view by deactivating its presenter and then hiding the view itself.
		 *
		 * @note This function calls the `onHide()` virtual method before hiding the view.
		 */
		void hideInner() override
		{
			deactivate();
			BaseViewType::hideInner();
		}

	  protected:
		std::shared_ptr<Presenter> m_presenter;
	};

} // namespace UI
