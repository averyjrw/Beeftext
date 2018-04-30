/// \file
/// \author Xavier Michelon
///
/// \brief Implementation of preferences manager
///  
/// Copyright (c) Xavier Michelon. All rights reserved.  
/// Licensed under the MIT License. See LICENSE file in the project root for full license information.  


#include "stdafx.h"
#include "PreferencesManager.h"
#include "I18nManager.h"
#include "InputManager.h"
#include "BeeftextUtils.h"
#include "BeeftextGlobals.h"
#include "BeeftextConstants.h"


namespace {
   QString const kKeyAlreadyLaunched = "AlreadyLaunched"; ///< The settings key for the "Already launched" indicator
   QString const kKeyFileMarkedForDeletion = "markedForDeletion"; ///< The path of the file marked for deletion on next application startup
   QString const kKeyGeometry = "Geometry"; ///< The settings key for storing the geometry
   QString const kKeyLocale = "Locale"; ///< The settings key for the locale
   QString const kKeyAppExePath = "AppExePath"; ///< The settings key for the application executable path
   QString const kKeyPlaySoundOnCombo = "PlaySoundOnCombo"; ///< The settings key for the 'Play sound on combo' preference
   QString const kKeyAutoStartAtLogin = "AutoStartAtLogin"; ///< The settings key for the 'Autostart at login' preference
   QString const kKeyAutoCheckForUpdates = "AutoCheckForUpdate"; ///< The settings key for the 'Autostart at login' preference
   QString const kKeyUseClipboardForComboSubstitution = "UseClipboardForComboSubstitution"; ///< The setting key for the 'Use clipboard for combo substitution' preference
   QString const kKeyUseCustomTheme = "UseCustomTheme"; ///< The setting key for the 'Use custom theme' preference
   QString const kKeyUseAutomaticSubstitution = "UseAutomaticSubstitution"; ///< The setting key for the 'Use automatic substitution' preference
   QString const kKeyLastUpdateCheckDateTime = "LastUpdateCheck"; ///< The setting key for the last update check date/time
   QString const kKeyComboListFolderPath = "ComboListFolderPath"; ///< The setting key for the combo list folder path
   QString const kKeyComboTriggerShortcutModifiers = "ComboTriggerShortcutModifiers"; ///< The setting key for the combo trigger shortcut modifiers
   QString const kKeyComboTriggerShortcutKeyCode = "ComboTriggerShortcutKeyCode"; ///< The setting key for the combo trigger shortcut key code
   QString const kKeyComboTriggerShortcutScanCode = "ComboTriggerShortcutScanCode"; ///< The setting key for the combo trigger shortcut scan code
   QString const kKeyAutoBackup = "AutoBackup"; ///< The setting key for the 'Auto backup' preference
   QString const kKeyLastComboImportExportPath = "LastComboImportExportPath"; ///< The setting key for 'Last combo import/export path' preference
   bool const kDefaultValuePlaySoundOnCombo = true; ///< The default value for the 'Play sound on combo' preference
   bool const kDefaultValueAutoStartAtLogin = false; ///< The default value for the 'Autostart at login' preference
   bool const kDefaultValueAutoCheckForUpdates = true; ///< The default value for the 'Auto check for update preference
   bool const kDefaultvalueUseClipboardForComboSubstitution = true; ///< The default value for the 'Use clipboard for combo substitution' preference
   bool const kDefaultValueUseCustomTheme = true; ///< The default value for the 'Use custom theme' preference
   bool const kDefaultValueUseAutomaticSubstitution = true; ///< The default value for the 'Use automatic substitution' preference
   bool const kDefaultValueAutoBackup = true; ///< The default value for the 'Auto backup' preference
   QString const kDefaultValueLastComboImportExportPath = QDir(QStandardPaths::writableLocation(
      QStandardPaths::DesktopLocation)).absoluteFilePath("Combos.json"); ///< The default value for the 'Last combo import/export path' preference
   SPShortcut const kDefaultValueComboTriggerShortcut = std::make_shared<Shortcut>(Qt::AltModifier | Qt::ShiftModifier 
      | Qt::ControlModifier, 'B', 48); ///< The default value for the 'combo trigger shortcut' preference
}


//**********************************************************************************************************************
/// This function returns the default value if the key does not exist in the settings OR if the value is not of the
/// expected data type.
///
/// \param[in] key The key to read from
/// \param[in] defaultValue The default value to use if the key does not exist or its value is not of the right type
/// \return The read value
//**********************************************************************************************************************
template <typename T> T PreferencesManager::readSettings(QString const& key, T const& defaultValue) const
{
   if (!settings_->contains(key))
      return defaultValue;
   QVariant v = settings_->value(key, QVariant::fromValue<T>(defaultValue));
   return v.canConvert<T>() ? qvariant_cast<T>(v) : defaultValue;
}


//**********************************************************************************************************************
/// \return The only allowed instance of the class
//**********************************************************************************************************************
PreferencesManager& PreferencesManager::instance()
{
   static PreferencesManager prefManager;
   return prefManager;
}


//**********************************************************************************************************************
/// \note We set the organization and application names manually in case we want to use the preferences before the call 
/// to QApplication::SetOrganizationName() and QApplication::SetApplicationName()
//**********************************************************************************************************************
PreferencesManager::PreferencesManager()
   : settings_(nullptr)
{
   // portable edition use a different storage method for preferences
      settings_ = isInPortableMode()
         ? std::make_unique<QSettings>(globals::portableModeSettingsFilePath(), QSettings::IniFormat) 
         : std::make_unique<QSettings>(constants::kOrganizationName, constants::kApplicationName);

   // Cache often accessed values
   cachedUseAutomaticSubstitution_ = this->readSettings<bool>(kKeyUseAutomaticSubstitution,
      kDefaultValueUseAutomaticSubstitution);
   this->cacheComboTriggerShortcut(); 

   // Some preferences setting need initialization
   this->applyCustomThemePreference();
   this->applyLocalePreference();
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void PreferencesManager::reset()
{
   this->setPlaySoundOnCombo(kDefaultValuePlaySoundOnCombo);
   this->setAutoCheckForUpdates(kDefaultValueAutoCheckForUpdates);
   this->setUseClipboardForComboSubstitution(kDefaultvalueUseClipboardForComboSubstitution);
   this->setUseCustomTheme(kDefaultValueUseCustomTheme);
   this->setUseAutomaticSubstitution(kDefaultValueUseAutomaticSubstitution);
   this->setComboTriggerShortcut(kDefaultValueComboTriggerShortcut);
   this->setAutoBackup(kDefaultValueAutoBackup);
   this->setLocale(I18nManager::instance().validateLocale(QLocale()));
   if (isInPortableMode())
   {
      this->setAutoStartAtLogin(kDefaultValueAutoStartAtLogin); // we do not actually touch the registry here
      this->setComboListFolderPath(globals::appDataDir());
   }
}


//**********************************************************************************************************************
/// The value for this preference is set by the NSIS installer. 
//**********************************************************************************************************************
QString PreferencesManager::getInstalledApplicationPath() const
{
   if (!settings_->contains(kKeyAppExePath))
      return QString();
   return QDir::fromNativeSeparators(this->readSettings<QString>(kKeyAppExePath));
}


//**********************************************************************************************************************
/// Set the settings value indicating that the application has been launched in the past
//**********************************************************************************************************************
void PreferencesManager::setAlreadyLaunched()
{
   settings_->setValue(kKeyAlreadyLaunched, 1);
}


//**********************************************************************************************************************
/// Check whether the application has ever been launched or not
//**********************************************************************************************************************
bool PreferencesManager::alreadyLaunched() const
{
   return this->readSettings<bool>(kKeyAlreadyLaunched, false);
}


//**********************************************************************************************************************
/// \param[in] path The path of the file to delete on next application startup
//**********************************************************************************************************************
void PreferencesManager::setFileMarkedForDeletionOnStartup(QString const& path)
{
   settings_->setValue(kKeyFileMarkedForDeletion, path);
}


//**********************************************************************************************************************
/// \return The path of the file marked for deletion on next application startup
//**********************************************************************************************************************
QString PreferencesManager::fileMarkedForDeletionOnStartup() const
{
   return readSettings<QString>(kKeyFileMarkedForDeletion);
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void PreferencesManager::clearFileMarkedForDeletionOnStartup()
{
   settings_->remove(kKeyFileMarkedForDeletion);
}


//**********************************************************************************************************************
/// \param[in] array The geometry as a byte array
//**********************************************************************************************************************
void PreferencesManager::setMainWindowGeometry(QByteArray const& array)
{
   settings_->setValue(kKeyGeometry, array);
}


//**********************************************************************************************************************
/// \return The geometry of the main window as a byte array
//**********************************************************************************************************************
QByteArray PreferencesManager::mainWindowGeometry() const
{
   return settings_->value(kKeyGeometry).toByteArray();
}


//**********************************************************************************************************************
/// \return the locale
//**********************************************************************************************************************
QLocale PreferencesManager::locale() const
{
   return I18nManager::instance().validateLocale(this->readSettings<QLocale>(kKeyLocale, QLocale::system()));
}


//**********************************************************************************************************************
/// \param[in] locale The locale
//**********************************************************************************************************************
void PreferencesManager::setLocale(QLocale const& locale)
{
   if (this->locale() != locale)
   {
      settings_->setValue(kKeyLocale, locale);
      this->applyLocalePreference();
   }
}


//**********************************************************************************************************************
/// \param[in] dateTime The date/time of the last update check
//**********************************************************************************************************************
void PreferencesManager::setLastUpdateCheckDateTime(QDateTime const& dateTime)
{
   settings_->setValue(kKeyLastUpdateCheckDateTime, dateTime);
}


//**********************************************************************************************************************
/// \return The date/time of the last update check
//**********************************************************************************************************************
QDateTime PreferencesManager::lastUpdateCheckDateTime() const
{
   QVariant v = settings_->value(kKeyLastUpdateCheckDateTime);
   return (v.isNull() || !v.canConvert<QDateTime>()) ? QDateTime(): v.toDateTime();
}


//**********************************************************************************************************************
/// \param[in] value The value for the preference
//**********************************************************************************************************************
void PreferencesManager::setAutoStartAtLogin(bool value)
{
   if (isInPortableMode())
   {
      globals::debugLog().addWarning("Trying to the set the 'auto start at login' preference while running "
         "in portable mode");
      return;
   }
   if (this->autoStartAtLogin() != value)
   {
      settings_->setValue(kKeyAutoStartAtLogin, value);
      this->applyAutoStartPreference();
   }
}


//**********************************************************************************************************************
/// \return The value for the preference
//**********************************************************************************************************************
bool PreferencesManager::autoStartAtLogin() const
{
   return isInPortableMode() ? false : this->readSettings<bool>(kKeyAutoStartAtLogin, kDefaultValueAutoStartAtLogin);
}


//**********************************************************************************************************************
/// \param[in] value The value for the preference
//**********************************************************************************************************************
void PreferencesManager::setPlaySoundOnCombo(bool value)
{
   settings_->setValue(kKeyPlaySoundOnCombo, value);
}


//**********************************************************************************************************************
/// \return The value for the preference
//**********************************************************************************************************************
bool PreferencesManager::playSoundOnCombo() const
{
   return this->readSettings<bool>(kKeyPlaySoundOnCombo, kDefaultValuePlaySoundOnCombo);
}


//**********************************************************************************************************************
/// \param[in] value The value for the preference
//**********************************************************************************************************************
void PreferencesManager::setAutoCheckForUpdates(bool value)
{
   if (this->autoCheckForUpdates() == value)
      return;
   settings_->setValue(kKeyAutoCheckForUpdates, value);
   emit autoCheckForUpdatesChanged(value);
}


//**********************************************************************************************************************
/// \return The value for the preference
//**********************************************************************************************************************
bool PreferencesManager::autoCheckForUpdates() const
{
   return this->readSettings<bool>(kKeyAutoCheckForUpdates, kDefaultValueAutoCheckForUpdates);
}


//**********************************************************************************************************************
/// \param[in] value The value for the preference
//**********************************************************************************************************************
void PreferencesManager::setUseClipboardForComboSubstitution(bool value)
{
   settings_->setValue(kKeyUseClipboardForComboSubstitution, value);
}


//**********************************************************************************************************************
/// \return The value for the preference
//**********************************************************************************************************************
bool PreferencesManager::useClipboardForComboSubstitution() const
{
   return this->readSettings<bool>(kKeyUseClipboardForComboSubstitution, kDefaultvalueUseClipboardForComboSubstitution);
}


//**********************************************************************************************************************
/// \param[in] value The value for the preference
//**********************************************************************************************************************
void PreferencesManager::setUseCustomTheme(bool value)
{
   if (this->useCustomTheme() != value)
   {
      settings_->setValue(kKeyUseCustomTheme, value);
      this->applyCustomThemePreference();
   }
}


//**********************************************************************************************************************
/// \return The value for the preference
//**********************************************************************************************************************
bool PreferencesManager::useCustomTheme() const
{
   return this->readSettings<bool>(kKeyUseCustomTheme, kDefaultValueUseCustomTheme);
}


//**********************************************************************************************************************
/// As the getter for this value is polled frequently (at every keystroke), it is cached
/// \param[in] value The new value for the preference
//**********************************************************************************************************************
void PreferencesManager::setUseAutomaticSubstitution(bool value)
{
   cachedUseAutomaticSubstitution_ = value;
   settings_->setValue(kKeyUseAutomaticSubstitution, value);
}


//**********************************************************************************************************************
/// \return The value for the preference
//**********************************************************************************************************************
bool PreferencesManager::useAutomaticSubstitution() const
{
   return cachedUseAutomaticSubstitution_;
}


//**********************************************************************************************************************
/// \param[in] value The value for the preference
//**********************************************************************************************************************
void PreferencesManager::setComboListFolderPath(QString const& value)
{
   if (isInPortableMode())
      globals::debugLog().addWarning("Trying to the set the 'combo list folder path' preference while running "
         "in portable mode");
   else
      settings_->setValue(kKeyComboListFolderPath, value);
}


//**********************************************************************************************************************
/// \return The value for the preference
//**********************************************************************************************************************
QString PreferencesManager::comboListFolderPath() const
{
   return isInPortableMode() ? globals::portableModeDataFolderPath() : 
      this->readSettings<QString>(kKeyComboListFolderPath, defaultComboListFolderPath());
}


//**********************************************************************************************************************
//// \return The path of the default combo list folder
//**********************************************************************************************************************
QString PreferencesManager::defaultComboListFolderPath()
{
   return isInPortableMode() ? globals::portableModeDataFolderPath() :globals::appDataDir();
}


//**********************************************************************************************************************
/// \param[in] value The value
//**********************************************************************************************************************
void PreferencesManager::setComboTriggerShortcut(SPShortcut const& value)
{
   SPShortcut newShortcut = value ? value : kDefaultValueComboTriggerShortcut;
   SPShortcut currentShortcut = comboTriggerShortcut();
   if (!currentShortcut)
      currentShortcut = kDefaultValueComboTriggerShortcut;
   if (*newShortcut != *currentShortcut)
   {
      settings_->setValue(kKeyComboTriggerShortcutModifiers, int(value->nativeModifiers()));
      settings_->setValue(kKeyComboTriggerShortcutKeyCode, value->nativeVirtualKey());
      settings_->setValue(kKeyComboTriggerShortcutScanCode, value->nativeScanCode());
      cachedComboTriggerShortcut_ = newShortcut;
   }
}


//**********************************************************************************************************************
/// \return The trigger shortcut
//**********************************************************************************************************************
SPShortcut PreferencesManager::comboTriggerShortcut() const
{
   return cachedComboTriggerShortcut_;
}


//**********************************************************************************************************************
/// \param[in] value The value for the preference
//**********************************************************************************************************************
void PreferencesManager::setAutoBackup(bool value)
{
   settings_->setValue(kKeyAutoBackup, value);
}


//**********************************************************************************************************************
/// \return The value for the preference
//**********************************************************************************************************************
bool PreferencesManager::autoBackup() const
{
   return this->readSettings<bool>(kKeyAutoBackup, kDefaultValueAutoBackup);
}


//**********************************************************************************************************************
/// \return The preference value
//**********************************************************************************************************************
QString PreferencesManager::lastComboImportExportPath() const
{
   return this->readSettings<QString>(kKeyLastComboImportExportPath, kDefaultValueLastComboImportExportPath);
}


//**********************************************************************************************************************
/// \param[in] path The preference value
//**********************************************************************************************************************
void PreferencesManager::setLastComboImportExportPath(QString const& path)
{
   settings_->setValue(kKeyLastComboImportExportPath, path);
}


//**********************************************************************************************************************
/// \return The default combo trigger shortcut
//**********************************************************************************************************************
SPShortcut PreferencesManager::defaultComboTriggerShortcut()
{
   return kDefaultValueComboTriggerShortcut;
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void PreferencesManager::cacheComboTriggerShortcut()
{
   int const intMods = this->readSettings<quint32>(kKeyComboTriggerShortcutModifiers, 0);
   quint32 vKey = this->readSettings<quint32>(kKeyComboTriggerShortcutKeyCode, 0);
   quint32 scanCode = this->readSettings<quint32>(kKeyComboTriggerShortcutScanCode, 0);
   if ((!intMods) || (!vKey) || (!scanCode))
      cachedComboTriggerShortcut_ =  kDefaultValueComboTriggerShortcut;
   else
   cachedComboTriggerShortcut_ = std::make_shared<Shortcut>(Qt::KeyboardModifiers(intMods), vKey, scanCode);
   if (!cachedComboTriggerShortcut_->isValid())
      cachedComboTriggerShortcut_ = kDefaultValueComboTriggerShortcut;
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void PreferencesManager::applyCustomThemePreference() const
{
   qApp->setStyleSheet(this->useCustomTheme() ? constants::kStyleSheet : QString());
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void PreferencesManager::applyAutoStartPreference() const
{
   if (isInPortableMode())
      return;
   if (this->autoStartAtLogin())
   {
      if (!registerApplicationForAutoStart())
         globals::debugLog().addWarning("Could not register the application for automatic startup on login.");
   }
   else
      unregisterApplicationFromAutoStart();
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void PreferencesManager::applyLocalePreference() const
{
   I18nManager::instance().setLocale(this->locale());
}



