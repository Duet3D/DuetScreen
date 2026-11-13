/*
 * UiTestSuite.h
 *
 *  Created on: 2025-08-14
 *      Author: Andy Everitt
 */

#pragma once

#include "test_utils/Screen.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "test_utils/TestSuite.h"
#include "test_utils/utils.h"
#include <gtest/gtest.h>

class UiTestSuiteInner : public TestSuite
{
  protected:
	UiTestSuiteInner();
	virtual ~UiTestSuiteInner();

	static UI::Xml::Paths xmlPaths();
};

class UiTestSuite : public UiTestSuiteInner
{
  protected:
	UiTestSuite();
	virtual ~UiTestSuite() = default;

	UI::Screen screen{"test_screen"};
};
