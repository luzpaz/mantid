#ifndef MANTID_CUSTOMINTERFACES_INDIRECTBAYESTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTBAYESTAB_H_

#include "MantidAPI/MatrixWorkspace.h"

#include <QMap>
#include <QtDoublePropertyManager>
#include <QtIntPropertyManager>
#include <QtTreePropertyBrowser>
#include <QWidget>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>


namespace MantidQt
{
	namespace CustomInterfaces
	{
		/**
			This class defines a abstract base class for the different tabs of the Indirect Bayes interface.
			Any joint functionality shared between each of the tabs should be implemented here as well as defining
			shared member functions.
    
			@author Samuel Jackson, STFC

			Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

			This file is part of Mantid.

			Mantid is free software; you can redistribute it and/or modify
			it under the terms of the GNU General Public License as published by
			the Free Software Foundation; either version 3 of the License, or
			(at your option) any later version.

			Mantid is distributed in the hope that it will be useful,
			but WITHOUT ANY WARRANTY; without even the implied warranty of
			MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
			GNU General Public License for more details.

			You should have received a copy of the GNU General Public License
			along with this program.  If not, see <http://www.gnu.org/licenses/>.

			File change history is stored at: <https://github.com/mantidproject/mantid>
			Code Documentation is available at: <http://doxygen.mantidproject.org>
		*/

		/// precision of double properties in bayes tabs
		static const unsigned int NUM_DECIMALS = 6;

		class DLLExport IndirectBayesTab : public QWidget
		{
			Q_OBJECT

		public:
			IndirectBayesTab(QWidget * parent = 0);
			~IndirectBayesTab();

			/// Returns a URL for the wiki help page for this interface
			QString tabHelpURL();

			/// Base methods implemented in derived classes 
			virtual QString help() = 0;
			virtual bool validate() = 0;
			virtual void run() = 0;

		signals:
			/// Send signal to parent window to execute python script
			void executePythonScript(const QString& pyInput, bool output);
			/// Send signal to parent window to show a message box to user
			void showMessageBox(const QString& message);

		protected:
			/// Function to plot a workspace to the miniplot using a workspace name
			void plotMiniPlot(const QString& workspace, size_t index);
			/// Function to plot a workspace to the miniplot using a workspace pointer
			void plotMiniPlot(const Mantid::API::MatrixWorkspace_const_sptr & workspace, size_t wsIndex);
			/// Function to run a string as python code
			void runPythonScript(const QString& pyInput);

			/// Plot of the input
			QwtPlot* m_plot;
			/// Curve on the plot
			QwtPlotCurve* m_curve;
			/// Tree of the properties
			QtTreePropertyBrowser* m_propTree;
			/// Internal list of the properties
			QMap<QString, QtProperty*> m_properties;
			/// Double manager to create properties
			QtDoublePropertyManager* m_dblManager;
			/// Int manager to create properties
			QtIntPropertyManager* m_intManager;

		};
	} // namespace CustomInterfaces
} // namespace Mantid

#endif