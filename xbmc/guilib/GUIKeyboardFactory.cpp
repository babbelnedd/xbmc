/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Application.h"
#include "ApplicationMessenger.h"
#include "LocalizeStrings.h"
#include "GUIKeyboardFactory.h"
#include "dialogs/GUIDialogOK.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "settings/Settings.h"
#include "utils/md5.h"
#include "utils/StringUtils.h"

#include "dialogs/GUIDialogKeyboardGeneric.h"
#if defined(TARGET_DARWIN_IOS)
#include "osx/ios/IOSKeyboard.h"
#endif

FILTERING CGUIKeyboardFactory::m_filtering = FILTERING_NONE;

CGUIKeyboardFactory::CGUIKeyboardFactory(void)
{
}

CGUIKeyboardFactory::~CGUIKeyboardFactory(void)
{}

void CGUIKeyboardFactory::keyTypedCB(CGUIKeyboard *ref, const std::string &typedString)
{
  if(ref)
  {
    // send our search message in safe way (only the active window needs it)
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, ref->GetWindowId(), 0);
    switch(m_filtering)
    {
      case FILTERING_SEARCH:
        message.SetParam1(GUI_MSG_SEARCH_UPDATE);
        message.SetStringParam(typedString);
        CApplicationMessenger::Get().SendGUIMessage(message, g_windowManager.GetActiveWindow());
        break;
      case FILTERING_CURRENT:
        message.SetParam1(GUI_MSG_FILTER_ITEMS);
        message.SetStringParam(typedString);
        CApplicationMessenger::Get().SendGUIMessage(message);
        break;
      case FILTERING_NONE:
        break;
    }
    ref->resetAutoCloseTimer();
  }
}

// Show keyboard with initial value (aTextString) and replace with result string.
// Returns: true  - successful display and input (empty result may return true or false depending on parameter)
//          false - unsucessful display of the keyboard or cancelled editing
bool CGUIKeyboardFactory::ShowAndGetInput(CStdString& aTextString, const CVariant &heading, bool allowEmptyResult, bool hiddenInput /* = false */, unsigned int autoCloseMs /* = 0 */)
{
  bool confirmed = false;
  CGUIKeyboard *kb = NULL;
  bool needsFreeing = true;
  //heading can be a string or a localization id
  std::string headingStr;
  if (heading.isString())
    headingStr = heading.asString();
  else if (heading.isInteger() && heading.asInteger())
    headingStr = g_localizeStrings.Get((uint32_t)heading.asInteger());

#if defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV2)
  kb = new CIOSKeyboard();
#endif

  if(!kb)
  {
    kb = (CGUIDialogKeyboardGeneric*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
    needsFreeing = false;
  }

  if(kb)
  {
    kb->startAutoCloseTimer(autoCloseMs);
    confirmed = kb->ShowAndGetInput(keyTypedCB, aTextString, aTextString, headingStr, hiddenInput);
    if(needsFreeing)
      delete kb;
  }

  if (confirmed)
  {
    if (!allowEmptyResult && aTextString.empty())
      confirmed = false;
  }

  return confirmed;
}

bool CGUIKeyboardFactory::ShowAndGetInput(CStdString& aTextString, bool allowEmptyResult, unsigned int autoCloseMs /* = 0 */)
{
  return ShowAndGetInput(aTextString, "", allowEmptyResult, false, autoCloseMs);
}

// Shows keyboard and prompts for a password.
// Differs from ShowAndVerifyNewPassword() in that no second verification is necessary.
bool CGUIKeyboardFactory::ShowAndGetNewPassword(CStdString& newPassword, const CVariant &heading, bool allowEmpty, unsigned int autoCloseMs /* = 0 */)
{
  return ShowAndGetInput(newPassword, heading, allowEmpty, true, autoCloseMs);
}

// Shows keyboard and prompts for a password.
// Differs from ShowAndVerifyNewPassword() in that no second verification is necessary.
bool CGUIKeyboardFactory::ShowAndGetNewPassword(CStdString& newPassword, unsigned int autoCloseMs /* = 0 */)
{
  return ShowAndGetNewPassword(newPassword, 12340, false, autoCloseMs);
}

bool CGUIKeyboardFactory::ShowAndGetFilter(CStdString &filter, bool searching, unsigned int autoCloseMs /* = 0 */)
{
  m_filtering = searching ? FILTERING_SEARCH : FILTERING_CURRENT;
  bool ret = ShowAndGetInput(filter, searching ? 16017 : 16028, true, false, autoCloseMs);
  m_filtering = FILTERING_NONE;
  return ret;
}


// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param newPassword Overwritten with user input if return=true.
// \param heading Heading to display
// \param allowEmpty Whether a blank password is valid or not.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIKeyboardFactory::ShowAndVerifyNewPassword(CStdString& newPassword, const CVariant &heading, bool allowEmpty, unsigned int autoCloseMs /* = 0 */)
{
  // Prompt user for password input
  CStdString userInput = "";
  if (!ShowAndGetInput(userInput, heading, allowEmpty, true, autoCloseMs))
  { // user cancelled, or invalid input
    return false;
  }
  // success - verify the password
  CStdString checkInput = "";
  if (!ShowAndGetInput(checkInput, 12341, allowEmpty, true, autoCloseMs))
  { // user cancelled, or invalid input
    return false;
  }
  // check the password
  if (checkInput == userInput)
  {
    XBMC::XBMC_MD5 md5state;
    md5state.append(userInput);
    md5state.getDigest(newPassword);
    StringUtils::ToLower(newPassword);
    return true;
  }
  CGUIDialogOK::ShowAndGetInput(12341, 12344, 0, 0);
  return false;
}

// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param strNewPassword Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIKeyboardFactory::ShowAndVerifyNewPassword(CStdString& newPassword, unsigned int autoCloseMs /* = 0 */)
{
  CStdString heading = g_localizeStrings.Get(12340);
  return ShowAndVerifyNewPassword(newPassword, heading, false, autoCloseMs);
}

// \brief Show keyboard and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIKeyboardFactory::ShowAndVerifyPassword(CStdString& strPassword, const CStdString& strHeading, int iRetries, unsigned int autoCloseMs /* = 0 */)
{
  CStdString strHeadingTemp;
  if (1 > iRetries && strHeading.size())
    strHeadingTemp = strHeading;
  else
    strHeadingTemp = StringUtils::Format("%s - %i %s",
                                         g_localizeStrings.Get(12326).c_str(),
                                         CSettings::Get().GetInt("masterlock.maxretries") - iRetries,
                                         g_localizeStrings.Get(12343).c_str());

  CStdString strUserInput = "";
  if (!ShowAndGetInput(strUserInput, strHeadingTemp, false, true, autoCloseMs))  //bool hiddenInput = false/true ? TODO: GUI Setting to enable disable this feature y/n?
    return -1; // user canceled out

  if (!strPassword.empty())
  {
    if (strPassword == strUserInput)
      return 0;

    CStdString md5pword2;
    XBMC::XBMC_MD5 md5state;
    md5state.append(strUserInput);
    md5state.getDigest(md5pword2);
    if (strPassword.Equals(md5pword2))
      return 0;     // user entered correct password
    else return 1;  // user must have entered an incorrect password
  }
  else
  {
    if (!strUserInput.empty())
    {
      XBMC::XBMC_MD5 md5state;
      md5state.append(strUserInput);
      md5state.getDigest(strPassword);
      StringUtils::ToLower(strPassword);
      return 0; // user entered correct password
    }
    else return 1;
  }
}

