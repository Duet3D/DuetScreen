/*
 * DataSources.h
 *
 * Starts and stops the data sources (see DataSource.h). The set of sources is fixed: it is the list in
 * DataSources.cpp, which is also the index of the printer data the UI API provides.
 */

#pragma once

namespace UI::Xml::DataSources
{
	/** Create every data source, publish its names globally, connect it to the Model and fill it */
	void start();

	/** Disconnect every data source from the Model, remove its global names and destroy it */
	void stop();

	/** Re-read every data source, e.g. after a language change, since some published text is translated */
	void updateAll();
} // namespace UI::Xml::DataSources
