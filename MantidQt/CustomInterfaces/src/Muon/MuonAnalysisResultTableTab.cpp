//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisResultTableTab.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidQtCustomInterfaces/Muon/MuonSequentialFitDialog.h"

#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QMessageBox>
#include <QDesktopServices>

#include <algorithm>

//-----------------------------------------------------------------------------

namespace {
/// The string "Error"
const static std::string ERROR_STRING("Error");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

const std::string MuonAnalysisResultTableTab::WORKSPACE_POSTFIX("_Workspace");
const std::string MuonAnalysisResultTableTab::PARAMS_POSTFIX("_Parameters");
const QString MuonAnalysisResultTableTab::RUN_NUMBER_LOG("run_number");
const QString MuonAnalysisResultTableTab::RUN_START_LOG("run_start");
const QString MuonAnalysisResultTableTab::RUN_END_LOG("run_end");
const QStringList MuonAnalysisResultTableTab::NON_TIMESERIES_LOGS{
    MuonAnalysisResultTableTab::RUN_NUMBER_LOG, "group", "period",
    RUN_START_LOG, RUN_END_LOG, "sample_temp", "sample_magn_field"};

/**
* Constructor
*/
MuonAnalysisResultTableTab::MuonAnalysisResultTableTab(Ui::MuonAnalysis &uiForm)
    : m_uiForm(uiForm), m_savedLogsState(), m_unselectedFittings() {
  // Connect the help button to the wiki page.
  connect(m_uiForm.muonAnalysisHelpResults, SIGNAL(clicked()), this,
          SLOT(helpResultsClicked()));

  // Set the default name
  m_uiForm.tableName->setText("ResultsTable");

  // Connect the select/deselect all buttons.
  connect(m_uiForm.selectAllLogValues, SIGNAL(toggled(bool)), this,
          SLOT(selectAllLogs(bool)));
  connect(m_uiForm.selectAllFittingResults, SIGNAL(toggled(bool)), this,
          SLOT(selectAllFittings(bool)));

  // Connect the create table button
  connect(m_uiForm.createTableBtn, SIGNAL(clicked()), this,
          SLOT(onCreateTableClicked()));

  // Enable relevant label combo-box only when matching fit type selected
  connect(m_uiForm.sequentialFit, SIGNAL(toggled(bool)), m_uiForm.fitLabelCombo,
          SLOT(setEnabled(bool)));
  connect(m_uiForm.simultaneousFit, SIGNAL(toggled(bool)),
          m_uiForm.cmbFitLabelSimultaneous, SLOT(setEnabled(bool)));

  // Re-populate tables when fit type or seq./sim. fit label is changed
  connect(m_uiForm.fitType, SIGNAL(buttonClicked(QAbstractButton *)), this,
          SLOT(populateTables()));
  connect(m_uiForm.fitLabelCombo, SIGNAL(activated(int)), this,
          SLOT(populateTables()));
  connect(m_uiForm.cmbFitLabelSimultaneous, SIGNAL(activated(int)), this,
          SLOT(populateTables()));
}

/**
* Muon Analysis Results Table Help (slot)
*/
void MuonAnalysisResultTableTab::helpResultsClicked() {
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
                                 "MuonAnalysisResultsTable"));
}

/**
* Select/Deselect all log values to be included in the table
*/
void MuonAnalysisResultTableTab::selectAllLogs(bool state) {
  if (state) {
    for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++) {
      QTableWidgetItem *temp =
          static_cast<QTableWidgetItem *>(m_uiForm.valueTable->item(i, 0));
      // If there is an item there then check the box
      if (temp != NULL) {
        QCheckBox *includeCell =
            static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(i, 1));
        includeCell->setChecked(true);
      }
    }
  } else {
    for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++) {
      QCheckBox *includeCell =
          static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(i, 1));
      includeCell->setChecked(false);
    }
  }
}

/**
* Select/Deselect all fitting results to be included in the table
*/
void MuonAnalysisResultTableTab::selectAllFittings(bool state) {
  if (state) {
    for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++) {
      QTableWidgetItem *temp = static_cast<QTableWidgetItem *>(
          m_uiForm.fittingResultsTable->item(i, 0));
      // If there is an item there then check the box
      if (temp != NULL) {
        QCheckBox *includeCell = static_cast<QCheckBox *>(
            m_uiForm.fittingResultsTable->cellWidget(i, 1));
        includeCell->setChecked(true);
      }
    }
  } else {
    for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++) {
      QCheckBox *includeCell = static_cast<QCheckBox *>(
          m_uiForm.fittingResultsTable->cellWidget(i, 1));
      includeCell->setChecked(false);
    }
  }
}

/**
 * Remembers which fittings and logs have been selected/deselected by the user.
 * Used in combination with
 * applyUserSettings() so that we dont lose what the user has chosen when
 * switching tabs.
 */
void MuonAnalysisResultTableTab::storeUserSettings() {
  m_savedLogsState.clear();

  // Find which logs have been selected by the user.
  for (int row = 0; row < m_uiForm.valueTable->rowCount(); ++row) {
    if (QTableWidgetItem *log = m_uiForm.valueTable->item(row, 0)) {
      QCheckBox *logCheckBox =
          static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(row, 1));
      m_savedLogsState[log->text()] = logCheckBox->checkState();
    }
  }

  m_unselectedFittings.clear();

  // Find which fittings have been deselected by the user.
  for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); ++row) {
    QTableWidgetItem *temp = m_uiForm.fittingResultsTable->item(row, 0);
    if (temp) {
      QCheckBox *fittingChoice = static_cast<QCheckBox *>(
          m_uiForm.fittingResultsTable->cellWidget(row, 1));
      if (!fittingChoice->isChecked())
        m_unselectedFittings += temp->text();
    }
  }
}

/**
 * Applies the stored lists of which fittings and logs have been
 * selected/deselected by the user.
 */
void MuonAnalysisResultTableTab::applyUserSettings() {
  // If we're just starting the tab for the first time (and there are no user
  // choices),
  // then don't bother.
  if (m_savedLogsState.isEmpty() && m_unselectedFittings.isEmpty())
    return;

  // If any of the logs have previously been selected by the user, select them
  // again.
  for (int row = 0; row < m_uiForm.valueTable->rowCount(); ++row) {
    if (QTableWidgetItem *log = m_uiForm.valueTable->item(row, 0)) {
      if (m_savedLogsState.contains(log->text())) {
        QCheckBox *logCheckBox =
            static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(row, 1));
        logCheckBox->setCheckState(m_savedLogsState[log->text()]);
      }
    }
  }

  // If any of the fittings have previously been deselected by the user,
  // deselect them again.
  for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); ++row) {
    QTableWidgetItem *temp = m_uiForm.fittingResultsTable->item(row, 0);
    if (temp) {
      if (m_unselectedFittings.contains(temp->text())) {
        QCheckBox *fittingChoice = static_cast<QCheckBox *>(
            m_uiForm.fittingResultsTable->cellWidget(row, 1));
        fittingChoice->setChecked(false);
      }
    }
  }
}

/**
 * Returns a list of workspaces which should be displayed in the table,
 * depending on what user has
 * chosen to view.
 * @return List of workspace base names
 */
QStringList MuonAnalysisResultTableTab::getFittedWorkspaces() {
  if (m_uiForm.fitType->checkedButton() == m_uiForm.individualFit) {
    return getIndividualFitWorkspaces();
  } else if (m_uiForm.fitType->checkedButton() == m_uiForm.sequentialFit) {
    QString selectedLabel = m_uiForm.fitLabelCombo->currentText();
    return getMultipleFitWorkspaces(selectedLabel, true);
  } else if (m_uiForm.fitType->checkedButton() == m_uiForm.simultaneousFit) {
    return getMultipleFitWorkspaces(
        m_uiForm.cmbFitLabelSimultaneous->currentText(), false);
  } else if (m_uiForm.fitType->checkedButton() == m_uiForm.multipleSimFits) {
    // all simultaneously fitted workspaces
    QStringList wsList;
    for (int i = 0; i < m_uiForm.cmbFitLabelSimultaneous->count(); ++i) {
      const auto names = getMultipleFitWorkspaces(
          m_uiForm.cmbFitLabelSimultaneous->itemText(i), false);
      wsList.append(names);
    }
    return wsList;
  } else {
    throw std::runtime_error("Unknown fit type option");
  }
}

/**
 * Returns a list of labels user has made sequential and simultaneous fits for.
 * @return Pair of lists of labels: <sequential, simultaneous>
 */
std::pair<QStringList, QStringList> MuonAnalysisResultTableTab::getFitLabels() {
  QStringList seqLabels, simLabels;

  std::map<std::string, Workspace_sptr> items =
      AnalysisDataService::Instance().topLevelItems();

  for (auto it = items.begin(); it != items.end(); ++it) {
    if (it->second->id() != "WorkspaceGroup")
      continue;

    if (it->first.find(MuonSequentialFitDialog::SEQUENTIAL_PREFIX) == 0) {
      std::string label =
          it->first.substr(MuonSequentialFitDialog::SEQUENTIAL_PREFIX.size());
      seqLabels << QString::fromStdString(label);
    } else if (it->first.find(MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX) ==
               0) {
      std::string label =
          it->first.substr(MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX.size());
      simLabels << QString::fromStdString(label);
    } else {
      continue;
    }
  }

  return std::make_pair(seqLabels, simLabels);
}

/**
 * Returns a list of sequentially/simultaneously fitted workspaces names.
 * @param label :: Label to return sequential/simultaneous fits for
 * @param sequential :: true for sequential, false for simultaneous
 * @return List of workspace base names
 */
QStringList
MuonAnalysisResultTableTab::getMultipleFitWorkspaces(const QString &label,
                                                     bool sequential) {
  const AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

  const std::string groupName = [&label, &sequential]() {
    if (sequential) {
      return MuonSequentialFitDialog::SEQUENTIAL_PREFIX + label.toStdString();
    } else {
      return MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX + label.toStdString();
    }
  }();

  WorkspaceGroup_sptr group;

  // Might have been accidentally deleted by user
  if (!ads.doesExist(groupName) ||
      !(group = ads.retrieveWS<WorkspaceGroup>(groupName))) {
    QMessageBox::critical(
        this, "Group not found",
        "Group with fitting results of the specified label was not found.");
    return QStringList();
  }

  std::vector<std::string> wsNames = group->getNames();

  QStringList workspaces;

  for (auto it = wsNames.begin(); it != wsNames.end(); it++) {
    if (!isFittedWs(*it))
      continue; // Doesn't pass basic checks

    workspaces << QString::fromStdString(wsBaseName(*it));
  }

  return workspaces;
}

/**
 * Returns a list individually fitted workspaces names.
 * @return List of workspace base names
 */
QStringList MuonAnalysisResultTableTab::getIndividualFitWorkspaces() {
  QStringList workspaces;

  auto allWorkspaces = AnalysisDataService::Instance().getObjectNames();

  for (auto it = allWorkspaces.begin(); it != allWorkspaces.end(); it++) {
    if (!isFittedWs(*it))
      continue; // Doesn't pass basic checks

    // Ignore sequential fit results
    if (boost::starts_with(*it, MuonSequentialFitDialog::SEQUENTIAL_PREFIX))
      continue;

    // Ignore simultaneous fit results
    if (boost::starts_with(*it, MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX)) {
      continue;
    }

    workspaces << QString::fromStdString(wsBaseName(*it));
  }

  return workspaces;
}

/**
 * Returns name of the fitted workspace with WORKSPACE_POSTFIX removed.
 * @param wsName :: Name of the fitted workspace. Shoud end with
 * WORKSPACE_POSTFIX.
 * @return wsName without WORKSPACE_POSTFIX
 */
std::string MuonAnalysisResultTableTab::wsBaseName(const std::string &wsName) {
  return wsName.substr(0, wsName.size() - WORKSPACE_POSTFIX.size());
}

/**
 * Does a few basic checks for whether the workspace is a fitted workspace.
 * @param wsName :: Name of the workspace to check for
 * @return True if seems to be fitted ws, false if doesn't
 */
bool MuonAnalysisResultTableTab::isFittedWs(const std::string &wsName) {
  if (!boost::ends_with(wsName, WORKSPACE_POSTFIX)) {
    return false; // Doesn't end with WORKSPACE_POSTFIX
  }

  try {
    auto ws = retrieveWSChecked<MatrixWorkspace>(wsName);

    ws->run().startTime();
    ws->run().endTime();
  } catch (...) {
    return false; // Not found / incorrect type / doesn't have start/end time
  }

  std::string baseName = wsBaseName(wsName);

  try {
    retrieveWSChecked<ITableWorkspace>(baseName + PARAMS_POSTFIX);
  } catch (...) {
    return false; // _Parameters workspace not found / has incorrect type
  }

  return true; // All OK
}

/**
 * Refresh the label lists and re-populate the tables.
 */
void MuonAnalysisResultTableTab::refresh() {
  m_uiForm.individualFit->setChecked(true);

  auto labels = getFitLabels();

  m_uiForm.fitLabelCombo->clear();
  m_uiForm.fitLabelCombo->addItems(labels.first);
  m_uiForm.cmbFitLabelSimultaneous->clear();
  m_uiForm.cmbFitLabelSimultaneous->addItems(labels.second);

  m_uiForm.sequentialFit->setEnabled(m_uiForm.fitLabelCombo->count() != 0);
  m_uiForm.simultaneousFit->setEnabled(
      m_uiForm.cmbFitLabelSimultaneous->count() > 0);
  m_uiForm.multipleSimFits->setEnabled(
      m_uiForm.cmbFitLabelSimultaneous->count() > 0);

  populateTables();
}

/**
 * Clear and populate both tables.
 */
void MuonAnalysisResultTableTab::populateTables() {
  storeUserSettings();

  // Clear the previous table values
  m_logValues.clear();
  m_uiForm.fittingResultsTable->setRowCount(0);
  m_uiForm.valueTable->setRowCount(0);

  QStringList fittedWsList = getFittedWorkspaces();
  fittedWsList.sort(); // sort by instrument then run (i.e. alphanumerically)

  if (!fittedWsList.isEmpty()) {
    // Populate the individual log values and fittings into their respective
    // tables.
    if (m_uiForm.fitType->checkedButton() == m_uiForm.multipleSimFits) {
      // Simultaneous fits: use labels
      auto wsFromName = [](const QString &qs) {
        const auto &wsGroup = retrieveWSChecked<WorkspaceGroup>(
            MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX + qs.toStdString());
        return boost::dynamic_pointer_cast<Workspace>(wsGroup);
      };
      populateFittings(getFitLabels().second, wsFromName);
    } else {
      // Use fitted workspace names
      auto wsFromName = [](const QString &qs) {
        const auto &tab = retrieveWSChecked<ITableWorkspace>(qs.toStdString() +
                                                             PARAMS_POSTFIX);
        return boost::dynamic_pointer_cast<Workspace>(tab);
      };
      populateFittings(fittedWsList, wsFromName);
    }

    populateLogsAndValues(fittedWsList);

    // Make sure all fittings are selected by default.
    selectAllFittings(true);

    // If we have Run Number log value, we want to select it by default.
    auto found =
        m_uiForm.valueTable->findItems("run_number", Qt::MatchFixedString);
    if (!found.empty()) {
      int r = found[0]->row();

      if (auto cb = dynamic_cast<QCheckBox *>(
              m_uiForm.valueTable->cellWidget(r, 1))) {
        cb->setCheckState(Qt::Checked);
      }
    }

    applyUserSettings();
  }
}

/**
* Populates the items (log values) into their table.
*
* @param fittedWsList :: a workspace list containing ONLY the workspaces that
* have parameter
*                   tables associated with it.
*/
void MuonAnalysisResultTableTab::populateLogsAndValues(
    const QStringList &fittedWsList) {
  // A set of all the logs we've met in the workspaces
  QSet<QString> allLogs;

  for (int i = 0; i < fittedWsList.size(); i++) {
    QMap<QString, QVariant> wsLogValues;

    // Get log information
    std::string wsName = fittedWsList[i].toStdString();
    auto ws = retrieveWSChecked<ExperimentInfo>(wsName + WORKSPACE_POSTFIX);

    Mantid::Kernel::DateAndTime start = ws->run().startTime();
    Mantid::Kernel::DateAndTime end = ws->run().endTime();

    const std::vector<Property *> &logData = ws->run().getLogData();

    for (const auto prop : logData) {
      // Check if is a timeseries log
      if (TimeSeriesProperty<double> *tspd =
              dynamic_cast<TimeSeriesProperty<double> *>(prop)) {
        QString logFile(QFileInfo(prop->name().c_str()).fileName());

        double value(0.0);
        int count(0);

        Mantid::Kernel::DateAndTime logTime;

        // iterate through all logs entries of a specific log
        for (int k(0); k < tspd->size(); k++) {
          // Get the log time for the specific entry
          logTime = tspd->nthTime(k);

          // If the entry was made during the run times
          if ((logTime >= start) && (logTime <= end)) {
            // add it to a total and increment the count (will be used to make
            // average entry value during a run)
            value += tspd->nthValue(k);
            count++;
          }
        }

        if (count != 0) {
          // Find average
          wsLogValues[logFile] = value / count;
        }
      } else // Should be a non-timeseries one
      {
        QString logName = QString::fromStdString(prop->name());

        // Check if we should display it
        if (NON_TIMESERIES_LOGS.contains(logName)) {

          if (logName == RUN_NUMBER_LOG) { // special case
            wsLogValues[RUN_NUMBER_LOG] =
                MuonAnalysisHelper::runNumberString(wsName, prop->value());
          } else if (logName == RUN_START_LOG || logName == RUN_END_LOG) {
            wsLogValues[logName + " (text)"] =
                QString::fromStdString(prop->value());
            const auto seconds = static_cast<double>(DateAndTime(prop->value())
                                                         .totalNanoseconds()) *
                                 1.e-9;
            wsLogValues[logName + " (s)"] = seconds;
          } else if (auto stringProp =
                         dynamic_cast<PropertyWithValue<std::string> *>(prop)) {
            wsLogValues[logName] = QString::fromStdString((*stringProp)());
          } else if (auto doubleProp =
                         dynamic_cast<PropertyWithValue<double> *>(prop)) {
            wsLogValues[logName] = (*doubleProp)();
          } else {
            throw std::runtime_error("Unsupported non-timeseries log type");
          }
        }
      }
    }

    // Append log names found in the workspace to the list of all known log
    // names
    allLogs += wsLogValues.keys().toSet();

    // Add all data collected from one workspace to another map. Will be used
    // when creating table.
    m_logValues[fittedWsList[i]] = wsLogValues;

  } // End loop over all workspace's log information and param information

  // Remove the logs that don't appear in all workspaces
  QSet<QString> toRemove;
  for (auto logIt = allLogs.constBegin(); logIt != allLogs.constEnd();
       ++logIt) {
    for (auto wsIt = m_logValues.constBegin(); wsIt != m_logValues.constEnd();
         ++wsIt) {
      auto wsLogValues = wsIt.value();
      if (!wsLogValues.contains(*logIt)) {
        toRemove.insert(*logIt);
        break;
      }
    }
  }

  allLogs = allLogs.subtract(toRemove);

  // Sort logs
  QList<QString> allLogsSorted(allLogs.toList());
  qSort(allLogsSorted.begin(), allLogsSorted.end(),
        MuonAnalysisResultTableTab::logNameLessThan);

  // Add number of rows to the table based on number of logs to display.
  m_uiForm.valueTable->setRowCount(allLogsSorted.size());

  // Populate table with all log values available without repeating any.
  for (auto it = allLogsSorted.constBegin(); it != allLogsSorted.constEnd();
       ++it) {
    int row = static_cast<int>(std::distance(allLogsSorted.constBegin(), it));
    m_uiForm.valueTable->setItem(row, 0, new QTableWidgetItem(*it));
  }

  // Add check boxes for the include column on log table, and make text
  // uneditable.
  for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++) {
    m_uiForm.valueTable->setCellWidget(i, 1, new QCheckBox);

    if (auto textItem = m_uiForm.valueTable->item(i, 0)) {
      textItem->setFlags(textItem->flags() & (~Qt::ItemIsEditable));
    }
  }
}

/**
 * LessThan function used to sort log names. Puts non-timeseries logs first and
 * the timeseries ones
 * sorted by named ignoring the case.
 * @param logName1
 * @param logName2
 * @return True if logName1 is less than logName2, false otherwise
 */
bool MuonAnalysisResultTableTab::logNameLessThan(const QString &logName1,
                                                 const QString &logName2) {
  int index1 = NON_TIMESERIES_LOGS.indexOf(logName1.split(' ').first());
  int index2 = NON_TIMESERIES_LOGS.indexOf(logName2.split(' ').first());

  if (index1 == -1 && index2 == -1) {
    // If both are timeseries logs - compare lexicographically ignoring the case
    return logName1.toLower() < logName2.toLower();
  } else if (index1 != -1 && index2 != -1) {
    // If both non-timeseries - keep the order of non-timeseries logs list
    if (index1 == index2) {
      // Correspond to same log, compare lexicographically
      return logName1.toLower() < logName2.toLower();
    } else {
      return index1 < index2;
    }
  } else {
    // If one is timeseries and another is not - the one which is not is always
    // less
    return index1 != -1;
  }
}

/**
 * Populates fittings table with given workspace/label names.
 * Can be used for workspace names that have associated param tables, or for
 * simultaneous fit labels, just by passing in a different function.
 *
 * @param names :: [input] list of workspace names OR label names
 * @param wsFromName :: [input] Function for getting workspaces from the given
 * names
 */
void MuonAnalysisResultTableTab::populateFittings(
    const QStringList &names,
    std::function<Workspace_sptr(const QString &)> wsFromName) {
  // Add number of rows for the amount of fittings.
  m_uiForm.fittingResultsTable->setRowCount(names.size());

  // Add check boxes for the include column on fitting table, and make text
  // uneditable.
  for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++) {
    m_uiForm.fittingResultsTable->setCellWidget(i, 1, new QCheckBox);

    if (auto textItem = m_uiForm.fittingResultsTable->item(i, 0)) {
      textItem->setFlags(textItem->flags() & (~Qt::ItemIsEditable));
    }
  }

  // Get workspace names using the provided function
  std::vector<Workspace_sptr> workspaces;
  for (const auto &name : names) {
    workspaces.push_back(wsFromName(name));
  }

  // Get colors for names in table
  const auto colors = MuonAnalysisHelper::getWorkspaceColors(workspaces);
  for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); row++) {
    // Fill values and delete previous old ones.
    if (row < names.size()) {
      QTableWidgetItem *item = new QTableWidgetItem(names[row]);
      item->setTextColor(colors.value(row));
      m_uiForm.fittingResultsTable->setItem(row, 0, item);
    } else
      m_uiForm.fittingResultsTable->setItem(row, 0, nullptr);
  }
}

void MuonAnalysisResultTableTab::onCreateTableClicked() {
  try {
    if (m_uiForm.fitType->checkedButton() == m_uiForm.multipleSimFits) {
      // Special results table for multiple simultaneous fits at once
      createMultipleFitsTable();
    } else {
      createTable(); // Regular results table
    }
  } catch (Exception::NotFoundError &e) {
    std::ostringstream errorMsg;
    errorMsg << "Workspace required to create a table was not found:\n\n"
             << e.what();
    QMessageBox::critical(this, "Workspace not found",
                          QString::fromStdString(errorMsg.str()));
    refresh(); // As something was probably deleted, refresh the tables
    return;
  } catch (std::exception &e) {
    std::ostringstream errorMsg;
    errorMsg << "Error occured when trying to create the table:\n\n"
             << e.what();
    QMessageBox::critical(this, "Error",
                          QString::fromStdString(errorMsg.str()));
    return;
  }
}

/**
* Creates the table using the information selected by the user in the tables
*/
void MuonAnalysisResultTableTab::createTable() {
  if (m_logValues.size() == 0) {
    QMessageBox::information(this, "Mantid - Muon Analysis",
                             "No workspace found with suitable fitting.");
    return;
  }

  // Get the user selection
  QStringList wsSelected = getSelectedItemsToFit();
  QStringList logsSelected = getSelectedLogs();

  if ((wsSelected.size() == 0) || logsSelected.size() == 0) {
    QMessageBox::information(this, "Mantid - Muon Analysis",
                             "Please select options from both tables.");
    return;
  }

  // Check workspaces have same parameters
  const auto tableFromName = [](const QString &qs) {
    return retrieveWSChecked<ITableWorkspace>(qs.toStdString() +
                                              PARAMS_POSTFIX);
  };
  if (!haveSameParameters(wsSelected, tableFromName)) {
    QMessageBox::information(
        this, "Mantid - Muon Analysis",
        "Please pick workspaces with the same fitted parameters");
    return;
  }
  // Create the results table
  Mantid::API::ITableWorkspace_sptr table =
      Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");

  // Add columns for log values
  foreach (QString log, logsSelected) {
    std::string columnTypeName;
    int columnPlotType;

    // We use values of the first workspace to determine the type of the
    // column to add. It seems reasonable to assume
    // that log values with the same name will have same types.
    QString typeName = m_logValues[wsSelected.first()][log].typeName();
    if (typeName == "double") {
      columnTypeName = "double";
      columnPlotType = 1;
    } else if (typeName == "QString") {
      columnTypeName = "str";
      columnPlotType = 6;
    } else
      throw std::runtime_error(
          "Couldn't find appropriate column type for value with type " +
          typeName.toStdString());

    Column_sptr newColumn = table->addColumn(columnTypeName, log.toStdString());
    newColumn->setPlotType(columnPlotType);
    newColumn->setReadOnly(false);
    }

    // Cache the start time of the first run
    int64_t firstStart_ns(0);
    if (const auto &firstWS =
            Mantid::API::AnalysisDataService::Instance()
                .retrieveWS<ExperimentInfo>(wsSelected.first().toStdString() +
                                            WORKSPACE_POSTFIX)) {
      firstStart_ns = firstWS->run().startTime().totalNanoseconds();
    }

    // Get param information
    QMap<QString, QMap<QString, double>> wsParamsList;
    QStringList paramsToDisplay;
    for (int i = 0; i < wsSelected.size(); ++i) {
      QMap<QString, double> paramsList;
      auto paramWs = retrieveWSChecked<ITableWorkspace>(
          wsSelected[i].toStdString() + PARAMS_POSTFIX);

      Mantid::API::TableRow paramRow = paramWs->getFirstRow();

      // Loop over all rows and get values and errors.
      do {
        std::string key;
        double value;
        double error;
        paramRow >> key >> value >> error;
        if (i == 0) {
          Column_sptr newValCol = table->addColumn("double", key);
          newValCol->setPlotType(2);
          newValCol->setReadOnly(false);

          Column_sptr newErrorCol = table->addColumn("double", key + ERROR_STRING);
          newErrorCol->setPlotType(5);
          newErrorCol->setReadOnly(false);

          paramsToDisplay.append(QString::fromStdString(key));
          paramsToDisplay.append(QString::fromStdString(key + ERROR_STRING));
        }
        paramsList[QString::fromStdString(key)] = value;
        paramsList[QString::fromStdString(key + ERROR_STRING)] = error;
      } while (paramRow.next());

      wsParamsList[wsSelected[i]] = paramsList;
    }

    // Add data to table
    for (const auto &wsName : wsSelected) {
      Mantid::API::TableRow row = table->appendRow();

      // Get log values for this row
      const auto &logValues = m_logValues[wsName];

      // Write log values in each column
      for (int i = 0; i < logsSelected.size(); ++i) {
        Mantid::API::Column_sptr col = table->getColumn(i);
        const QVariant &v = logValues[logsSelected[i]];

        if (col->isType<double>()) {
          if (logsSelected[i].endsWith(" (s)")) {
            // Convert relative to first start time
            double seconds = v.toDouble();
            const double firstStart_sec =
                static_cast<double>(firstStart_ns) * 1.e-9;
            row << seconds - firstStart_sec;
          } else {
            row << v.toDouble();
          }
        } else if (col->isType<std::string>()) {
          row << v.toString().toStdString();
        } else {
          throw std::runtime_error(
              "Log value with name '" + logsSelected[i].toStdString() +
              "' in '" + wsName.toStdString() + "' has unexpected type.");
        }
      }

      // Add param values (params same for all workspaces)
      QMap<QString, double> paramsList = wsParamsList[wsName];
      for (const auto &paramName : paramsToDisplay) {
        row << paramsList[paramName];
      }
    }

    // Remove error columns if all errors are zero
    // (because these correspond to fixed parameters)
    MuonAnalysisHelper::removeFixedParameterErrors(table);

    std::string tableName = getFileName();

    // Save the table to the ADS
    Mantid::API::AnalysisDataService::Instance().addOrReplace(tableName, table);

    // Python code to show a table on the screen
    std::stringstream code;
    code << "found = False\n"
         << "for w in windows():\n"
         << "  if w.windowLabel() == '" << tableName << "':\n"
         << "    found = True; w.show(); w.setFocus()\n"
         << "if not found:\n"
         << "  importTableWorkspace('" << tableName << "', True)\n";

    emit runPythonCode(QString::fromStdString(code.str()), false);
}

/**
 * Creates the results table for multiple simultaneous fit labels at once
 * and displays it, using user's selected logs/labels
 */
void MuonAnalysisResultTableTab::createMultipleFitsTable() {
    if (m_logValues.size() == 0) {
    QMessageBox::information(this, "Mantid - Muon Analysis",
                             "No workspace found with suitable fitting.");
    return;
  }

  // Get the user selection
  QStringList labelsSelected = getSelectedItemsToFit();
  QStringList logsSelected = getSelectedLogs();

  if ((labelsSelected.size() == 0) || logsSelected.size() == 0) {
    QMessageBox::information(this, "Mantid - Muon Analysis",
                             "Please select options from both tables.");
    return;
  }

  // Get the workspaces corresponding to the selected labels
  std::map<QString, std::vector<std::string>> workspacesByLabel;
  for (const auto &label : labelsSelected) {
    std::vector<std::string> wsNames;
    const auto &group = retrieveWSChecked<WorkspaceGroup>(
        MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX + label.toStdString());
    for (const auto &name : group->getNames()) {
      const size_t pos = name.find(WORKSPACE_POSTFIX);
      if (pos != std::string::npos) {
        wsNames.push_back(name.substr(0, pos));
      }
    }
    if (wsNames.empty()) {
      // This guarantees the list of workspaces for each label will not be empty
      QMessageBox::information(
          this, "Mantid - Muon Analysis",
          QString("No fitted workspaces found for label ").append(label));
      return;
    }
    workspacesByLabel[label] = wsNames;
  }

  // Lambda to find parameter table in the group for a given label
  const auto tableFromName = [](const QString &qs) {
    const auto &wsGroup = retrieveWSChecked<WorkspaceGroup>(
        MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX + qs.toStdString());
    for (const auto &name : wsGroup->getNames()) {
      if (name.find(PARAMS_POSTFIX) != std::string::npos) {
        return boost::dynamic_pointer_cast<ITableWorkspace>(
            wsGroup->getItem(name));
      }
    }
    return ITableWorkspace_sptr();
  };

  // Check workspaces have the same parameters
  if (!haveSameParameters(labelsSelected, tableFromName)) {
    QMessageBox::information(
        this, "Mantid - Muon Analysis",
        "Please pick workspaces with the same fitted parameters");
    return;
  }

  // Check fits have the same number of runs
  const size_t firstNumRuns = workspacesByLabel.begin()->second.size();
  if (std::any_of(workspacesByLabel.begin(), workspacesByLabel.end(),
                  [&firstNumRuns](
                      const std::pair<QString, std::vector<std::string>> fit) {
                    return fit.second.size() != firstNumRuns;
                  })) {
    QMessageBox::information(
        this, "Mantid - Muon Analysis",
        "Please pick fit labels with the same number of workspaces");
    return;
  }

  // Create the results table
  Mantid::API::ITableWorkspace_sptr table =
      Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");

  // Add column for label
  auto labelCol = table->addColumn("str", "Label");
  labelCol->setPlotType(6);
  labelCol->setReadOnly(false);

  // Add columns for log values
  foreach (QString log, logsSelected) {
    // Despite type "str", this will still work for plotting if you put a number
    // in it (like "100")
    Column_sptr newColumn = table->addColumn("str", log.toStdString());
    newColumn->setPlotType(1);
    newColumn->setReadOnly(false);
  }

  // Cache the start time of the first run. We don't know which label was first,
  // so test them all.
  int64_t firstStart_ns = std::numeric_limits<int64_t>::max();
  for (const auto &label : labelsSelected) {
    const auto &wsName = workspacesByLabel[label].front();
    if (const auto &ws =
            Mantid::API::AnalysisDataService::Instance()
                .retrieveWS<ExperimentInfo>(wsName + WORKSPACE_POSTFIX)) {
      const int64_t start_ns = ws->run().startTime().totalNanoseconds();
      if (start_ns < firstStart_ns) {
        firstStart_ns = start_ns;
      }
    }
  }

  // Get param information
  QMap<QString, WSParameterList> wsParamsByLabel;
  for (const auto &label : labelsSelected) {
    WSParameterList wsParamsList;
    for (size_t i = 0; i < workspacesByLabel[label].size(); ++i) {
      QMap<QString, double> paramsList;
      auto paramWs = retrieveWSChecked<ITableWorkspace>(
          workspacesByLabel[label][i] + PARAMS_POSTFIX);

      Mantid::API::TableRow paramRow = paramWs->getFirstRow();

      // Loop over all rows and get values and errors.
      do {
        std::string key;
        double value;
        double error;
        paramRow >> key >> value >> error;
        paramsList[QString::fromStdString(key)] = value;
        paramsList[QString::fromStdString(key + ERROR_STRING)] = error;
      } while (paramRow.next());

      wsParamsList[QString::fromStdString(workspacesByLabel[label][i])] =
          paramsList;
    }
    wsParamsByLabel[label] = wsParamsList;
  }

  // Add columns to table
  QStringList paramsToDisplay;
  auto paramNames = wsParamsByLabel.begin()->begin()->keys();
  // Remove the errors and cost function - just want the parameters
  paramNames.erase(std::remove_if(paramNames.begin(), paramNames.end(),
                                  [](const QString &qs) {
                                    return qs.endsWith("Error") ||
                                           qs.startsWith("Cost function");
                                  }),
                   paramNames.end());

  // Add a new column (with error) to table
  const auto addToTable = [&table, &paramsToDisplay](const std::string &key) {
    Column_sptr newValCol = table->addColumn("double", key);
    newValCol->setPlotType(2);
    newValCol->setReadOnly(false);
    const std::string keyError = key + ERROR_STRING;
    Column_sptr newErrorCol = table->addColumn("double", keyError);
    newErrorCol->setPlotType(5);
    newErrorCol->setReadOnly(false);
    paramsToDisplay.append(QString::fromStdString(key));
    paramsToDisplay.append(QString::fromStdString(keyError));
  };

  // Global: add one column
  // Local: add one per dataset
  for (const auto &param : paramNames) {
    if (isGlobal(param, wsParamsByLabel)) {
      addToTable(param.toStdString());
    } else {
      const int nDatasetsPerLabel = wsParamsByLabel.begin()->count();
      for (int i = 0; i < nDatasetsPerLabel; ++i) {
        addToTable(('f' + QString::number(i) + '.' + param).toStdString());
      }
    }
  }

  // -------------------testing ----------------

  // Add data to table
  for (const auto &labelName : labelsSelected) {
    Mantid::API::TableRow row = table->appendRow();

    row << labelName.toStdString();

    // Get log values for this row and write in table
    for (const auto &log : logsSelected) {
      QStringList valuesPerWorkspace;
      for (const auto &wsName : workspacesByLabel[labelName]) {
        const auto &logValues = m_logValues[QString::fromStdString(wsName)];
        const auto &val = logValues[log];

        // Special case: if log is time in sec, subtract the first start time
        if (log.endsWith(" (s)")) {
          auto seconds =
              val.toDouble() - static_cast<double>(firstStart_ns) * 1.e-9;
          valuesPerWorkspace.append(QString::number(seconds));
        } else {
          valuesPerWorkspace.append(logValues[log].toString());
        }
      }

      // Range of values - use string comparison as works for numbers too
      // Why not use std::minmax_element? To avoid MSVC warning: QT bug 41092
      // (https://bugreports.qt.io/browse/QTBUG-41092)
      valuesPerWorkspace.sort();
      const auto &min = valuesPerWorkspace.front().toStdString();
      const auto &max = valuesPerWorkspace.back().toStdString();
      if (min == max) {
        row << min;
      } else {
        std::ostringstream oss;
        oss << min << '-' << max;
        row << oss.str();
      }
    }

    //// Add param values (params same for all workspaces)
    // QMap<QString, double> paramsList = wsParamsList[wsName];
    // for (const auto &paramName : paramsToDisplay) {
    //  row << paramsList[paramName];
    //}
  }

  // Remove error columns if all errors are zero
  // (because these correspond to fixed parameters)
  MuonAnalysisHelper::removeFixedParameterErrors(table);

  //////////////////////////// code to go here

  const std::string &tableName = getFileName();

  // Save the table to the ADS
  Mantid::API::AnalysisDataService::Instance().addOrReplace(tableName, table);

  // Python code to show a table on the screen
  std::stringstream code;
  code << "found = False\n"
       << "for w in windows():\n"
       << "  if w.windowLabel() == '" << tableName << "':\n"
       << "    found = True; w.show(); w.setFocus()\n"
       << "if not found:\n"
       << "  importTableWorkspace('" << tableName << "', True)\n";

  emit runPythonCode(QString::fromStdString(code.str()), false);
}

/**
* See if the workspaces/labels selected have the same parameters.
*
* @param names :: A list of workspaces with fitted parameters OR labels.
* @param tableFromName :: Function to get fit table from any given name.
* @return bool :: Whether or not the names given share the same fitting
* parameters.
*/
bool MuonAnalysisResultTableTab::haveSameParameters(
    const QStringList &names,
    std::function<ITableWorkspace_sptr(const QString &)> tableFromName) {
  std::vector<ITableWorkspace_sptr> tables;
  for (const auto &name : names) {
    tables.push_back(tableFromName(name));
  }

  return MuonAnalysisHelper::haveSameParameters(tables);
}

/**
 * Get the user selected workspaces OR labels from the table
 * @return :: list of selected workspaces/labels
 */
QStringList MuonAnalysisResultTableTab::getSelectedItemsToFit() {
  QStringList items;
  for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); ++i) {
    const auto includeCell = static_cast<QCheckBox *>(
        m_uiForm.fittingResultsTable->cellWidget(i, 1));
    if (includeCell->isChecked()) {
      items.push_back(m_uiForm.fittingResultsTable->item(i, 0)->text());
    }
  }
  return items;
}

/**
* Get the user selected log file
*
* @return logsSelected :: A vector of QString's containing the logs that are
* selected.
*/
QStringList MuonAnalysisResultTableTab::getSelectedLogs() {
  QStringList logsSelected;
  for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++) {
    QCheckBox *includeCell =
        static_cast<QCheckBox *>(m_uiForm.valueTable->cellWidget(i, 1));
    if (includeCell->isChecked()) {
      QTableWidgetItem *logParam =
          static_cast<QTableWidgetItem *>(m_uiForm.valueTable->item(i, 0));
      logsSelected.push_back(logParam->text());
    }
  }
  return logsSelected;
}

/**
* Checks that the file name isn't been used, displays the appropriate message
* and
* then returns the name in which to save.
*
* @return name :: The name the results table should be created with.
*/
std::string MuonAnalysisResultTableTab::getFileName() {
  std::string fileName(m_uiForm.tableName->text().toStdString());

  if (Mantid::API::AnalysisDataService::Instance().doesExist(fileName)) {
    int choice = QMessageBox::question(
        this, tr("MantidPlot - Overwrite Warning"),
        QString::fromStdString(fileName) +
            tr(" already exists. Do you want to replace it?"),
        QMessageBox::Yes | QMessageBox::Default,
        QMessageBox::No | QMessageBox::Escape);
    if (choice == QMessageBox::No) {
      int versionNum(2);
      fileName += " #";
      while (Mantid::API::AnalysisDataService::Instance().doesExist(
          fileName + boost::lexical_cast<std::string>(versionNum))) {
        versionNum += 1;
      }
      return (fileName + boost::lexical_cast<std::string>(versionNum));
    }
  }
  return fileName;
}

/**
 * In the supplied list of fit results, finds if the given parameter appears to
 * have been global. The global params have the same value for all workspaces in
 * a label.
 * @param param :: [input] Name of parameter
 * @param paramList :: [input] Map of label name to "WSParameterList" (itself a
 * map of workspace name to <name, value> map)
 * @returns :: Whether the parameter was global
 */
bool MuonAnalysisResultTableTab::isGlobal(
    const QString &param, const QMap<QString, WSParameterList> &paramList) {
  // It is safe to assume the same fit model was used for all labels.
  // So just test the first:
  return isGlobal(param, *paramList.begin());
}

/**
 * In the supplied list of fit results, finds if the given parameter appears to
 * have been global. The global params have the same value for all workspaces.
 * @param param :: [input] Name of parameter
 * @param paramList :: [input] Map of workspace name to <name, value> map
 * @returns :: Whether the parameter was global
 */
bool MuonAnalysisResultTableTab::isGlobal(const QString &param,
                                          const WSParameterList &paramList) {
  const double firstValue = paramList.begin()->value(param);
  if (paramList.size() > 1) {
    for (auto it = paramList.begin() + 1; it != paramList.end(); ++it) {
      // If any parameter differs from the first it cannot be global
      if (std::abs(it->value(param) - firstValue) >
          std::numeric_limits<double>::epsilon()) {
        return false;
      }
    }
    return true; // All values are the same so it is global
  } else {
    return false; // Only one workspace so no globals
  }
}
}
}
}
