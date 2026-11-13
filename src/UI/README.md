# UI

The UI uses a Model View Presenter (MVP) structure. The model is the data that the UI is displaying, the view is the UI itself, and the presenter is the logic that connects the model to the view. The UI is built using the LittlevGL library, which is a C library for creating embedded GUIs.

Some general rules are:
- A `View` must not contain any logic that is not directly related to the UI. This includes things like data fetching, data manipulation, etc.
- Updating the UI should only be done through the `Presenter`, which will call the appropriate methods on the `View`.
- Only 1 `Model` may be instantiated at a time. This is because the `Model` is responsible for storing the data that the UI is displaying, and having multiple `Model`s would mean that the UI is displaying multiple sets of data at once.
- Each `View` has its own `Presenter` which is responsible for updating the `View` with the data from the `Model`.
- User interaction with the `View` should be handled by the `Presenter`, which will then update the `Model` and the `View` as necessary.

The UI is split into multiple sections, each of which is responsible for a different aspect of the UI. The sections are as follows:
- [Components](./Components/README.md)
- [Core](./Core/README.md)
- [Screens](./Screens/README.md)
- [Styles](./Styles/README.md)

## Components
These are reusable components that can be used in multiple screens.
A component is "dumb", or it can have some business logic associated with it. Interactivity is not defined by the component itself but is instead passed in as a callback by the view that uses the component.
- For example, a button component should not contain the logic for what happens when the button is clicked, but should instead call a callback function that is passed in as a prop.
- These components should inherit from the `BaseView` class

## Core
This is where the MVP structure is implemented. It contains the base `Model`, `View`, and `Presenter` classes which will be inherited by the other classes in the UI.

## Screens
These are the different screens that the user can navigate to. Each screen will have its own `View`, and `Presenter` classes.

## Styles
This is where the styles for the UI are defined. This includes things like colors, fonts, and sizes.

## Widgets
Like a [component](#components) but has its own `Presenter` class that will handle the logic for the widget.
- For example, a component that displays a list of items may have a `Presenter` that fetches the data to display in the list.
- Widgets should inherit from the `View` class, which takes a `Presenter` and an optional `BaseViewType` as template arguments