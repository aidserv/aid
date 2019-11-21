// Copyright (C) 2015-2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/telephony-page.h"

#include "android/emulation/control/telephony_agent.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/telephony/modem.h"
#include "android/telephony/sms.h"
#include <QSettings>
#include <cassert>
#include <cctype>

#define MAX_SMS_MSG_SIZE 1024 // Arbitrary emulator limitation
#define MAX_SMS_MSG_SIZE_STRING "1024"

class TelephonyEvent : public QEvent {
public:
    TelephonyEvent(QEvent::Type typeId, int nActive) :
        QEvent(typeId),
        numActiveCalls(nActive) { }
public:
    int numActiveCalls;
};

TelephonyPage::TelephonyPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::TelephonyPage()),
    mTelephonyAgent(nullptr),
    mCallActivity(TelephonyPage::CallActivity::Inactive)
{
    mUi->setupUi(this);
    mUi->tel_numberBox->setValidator(new PhoneNumberValidator());
    mCustomEventType = (QEvent::Type)QEvent::registerEventType();
}

TelephonyPage::~TelephonyPage() {
    if (mTelephonyAgent  &&  mTelephonyAgent->setNotifyCallback) {
        // Tell the agent that we do not want any more call-backs
        mTelephonyAgent->setNotifyCallback(0, 0);
    }
}

void TelephonyPage::on_tel_startEndButton_clicked()
{
    QSettings settings;
    SettingsTheme theme =
        (SettingsTheme)settings.value(Ui::Settings::UI_THEME, 0).toInt();
    if (mCallActivity == CallActivity::Inactive) {
        // Start a call
        if (mTelephonyAgent && mTelephonyAgent->telephonyCmd) {
            // Command the emulator
            TelephonyResponse tResp;

            // Get rid of spurious characters from the phone number
            // (Allow only '+' and '0'..'9')
            // Note: phoneNumberValidator validates the user's input, but
            // allows some human-readable characters like '.' and ')'.
            // Here we remove that meaningless punctuation.
            QString cleanNumber = mUi->tel_numberBox->currentText().
                                     remove(QRegularExpression("[^+0-9]"));

            tResp = mTelephonyAgent->telephonyCmd(Tel_Op_Init_Call,
                                                  cleanNumber.toStdString().c_str());
            if (tResp != Tel_Resp_OK) {
                showErrorDialog(tr("The call failed."), tr("Telephony"));
                return;
            }
        }

        // Success: Update the state and the UI buttons
        mCallActivity = CallActivity::Active;
        mPhoneNumber = mUi->tel_numberBox->currentText();

        mUi->tel_numberBox->setEnabled(false);

        setButtonEnabled(mUi->tel_holdCallButton, theme, true);
        // Change the icon and text to "END CALL"
        QIcon theIcon((theme == SETTINGS_THEME_DARK) ? ":/dark/call_end"
                                                     : ":/light/call_end");
        mUi->tel_startEndButton->setIcon(theIcon);
        mUi->tel_startEndButton->setText(tr("END CALL"));
    } else {
        // End a call
        // Update the state and the UI buttons
        mCallActivity = CallActivity::Inactive;

        mUi->tel_numberBox->setEnabled(true);

        mUi->tel_holdCallButton->setProperty("themeIconName", "phone_paused");

        setButtonEnabled(mUi->tel_holdCallButton, theme, false);
        // Change the icon and text to "CALL DEVICE"
        QIcon theIcon((theme == SETTINGS_THEME_DARK) ? ":/dark/phone_button"
                                                     : ":/light/phone_button");
        mUi->tel_startEndButton->setIcon(theIcon);
        mUi->tel_startEndButton->setText(tr("CALL DEVICE"));

        if (mTelephonyAgent && mTelephonyAgent->telephonyCmd) {
            // Command the emulator
            QString cleanNumber = mUi->tel_numberBox->currentText().
                                    remove(QRegularExpression("[^+0-9]"));
            TelephonyResponse tResp;
            tResp = mTelephonyAgent->telephonyCmd(Tel_Op_Disconnect_Call,
                                                  cleanNumber.toStdString().c_str());
            if (tResp != Tel_Resp_OK  &&
                tResp != Tel_Resp_Invalid_Action)
            {
                // Don't show an error for Invalid Action: that
                // just means that the AVD already hanged up.
                showErrorDialog(tr("The end-call failed."), tr("Telephony"));
                return;
            }
        }
    }
}

void TelephonyPage::on_tel_holdCallButton_clicked()
{
    QSettings settings;
    SettingsTheme theme =
        (SettingsTheme)settings.value(Ui::Settings::UI_THEME, 0).toInt();
    switch (mCallActivity) {
        case CallActivity::Active:
            // Active --> On hold
            mCallActivity = CallActivity::Held;

            mUi->tel_holdCallButton->
                setProperty("themeIconName", "phone_in_talk");

            setButtonEnabled(mUi->tel_holdCallButton,  theme, true);
            break;
        case CallActivity::Held:
            // On hold --> Active
            mCallActivity = CallActivity::Active;

            mUi->tel_holdCallButton->
                setProperty("themeIconName",          "phone_paused");
            mUi->tel_holdCallButton->
                setProperty("themeIconName_disabled", "phone_paused_disabled");

            setButtonEnabled(mUi->tel_holdCallButton,  theme, true);
            break;
        default:;
    }
    if (mTelephonyAgent && mTelephonyAgent->telephonyCmd) {
        // Command the emulator
        QString cleanNumber = mUi->tel_numberBox->currentText().
                                remove(QRegularExpression("[^+0-9]"));
        TelephonyResponse  tResp;
        TelephonyOperation tOp = (mCallActivity == CallActivity::Held) ?
                                     Tel_Op_Place_Call_On_Hold :
                                     Tel_Op_Take_Call_Off_Hold;

        tResp = mTelephonyAgent->telephonyCmd(tOp,
                                              cleanNumber.toStdString().c_str());
        if (tResp != Tel_Resp_OK) {
            showErrorDialog(tr("The call hold failed."), tr("Telephony"));
            return;
        }
    }
}

QValidator::State TelephonyPage::PhoneNumberValidator::validate(QString &input, int &pos) const
{
    int numDigits = 0;
    const int MAX_DIGITS = 16;
    static const QString acceptable_non_digits = "-.()/ +";

    if (input.length() >= 32) {
        return QValidator::Invalid;
    }

    for (int i = 0; i < input.length(); i++) {
        QCharRef c = input[i];
        if (c.isDigit()) {
            numDigits++;
            if (numDigits > MAX_DIGITS) {
                return QValidator::Invalid;
            }
        } else if (c == '+' && i != 0) {
            // '+' is only allowed as the first character
            return QValidator::Invalid;
        } else if (!acceptable_non_digits.contains(c)) {
            return QValidator::Invalid;
        }
    }

    return ((numDigits > 0) ? QValidator::Acceptable : QValidator::Intermediate);
}

void TelephonyPage::on_sms_sendButton_clicked()
{
    // Get the "from" number
    SmsAddressRec sender;
    int retVal = sms_address_from_str(&sender,
                     mUi->tel_numberBox->
                              currentText().toStdString().c_str(),
                     mUi->tel_numberBox->
                              currentText().length());
    if (retVal < 0  ||  sender.len <= 0) {
        showErrorDialog(tr("The \"From\" number is invalid."), tr("SMS"));
        return;
    }

    // Get the text of the message
    const std::string theMessage =
            mUi->sms_messageBox->toPlainText().toStdString();

    // Convert the message text to UTF-8
    unsigned char utf8Message[MAX_SMS_MSG_SIZE+1];
    int           nUtf8Chars;
    nUtf8Chars = sms_utf8_from_message_str(theMessage.c_str(),
                                           theMessage.size(),
                                           utf8Message,
                                           MAX_SMS_MSG_SIZE);
    if (nUtf8Chars == 0) {
        showErrorDialog(tr("The message is empty.<br>Please enter a message."), tr("SMS"));
        return;
    }

    if (nUtf8Chars < 0) {
        showErrorDialog(tr("The message contains invalid characters."), tr("SMS"));
        return;
    }

    if (nUtf8Chars > MAX_SMS_MSG_SIZE) {
        // Note: 'sms_utf8_from_message_str' did not overwrite our buffer
        showErrorDialog(tr("Android Emulator truncated this message to "
                           MAX_SMS_MSG_SIZE_STRING " characters."), tr("SMS"));
        nUtf8Chars = MAX_SMS_MSG_SIZE;
    }

    // Create a list of SMS PDUs, then send them
    SmsPDU *pdus = smspdu_create_deliver_utf8(utf8Message, nUtf8Chars,
                                              &sender, NULL);
    if (pdus == NULL) {
        showErrorDialog(tr("The message contains invalid characters."),
                                     tr("SMS"));
        return;
    }

    if (mTelephonyAgent && mTelephonyAgent->getModem) {
        AModem modem = mTelephonyAgent->getModem();
        if (modem == NULL) {
            showErrorDialog(tr("Cannot send message, modem emulation not running."), tr("SMS"));
            return;
        }

        for (int idx = 0; pdus[idx] != NULL; idx++) {
            amodem_receive_sms(modem, pdus[idx]);
        }
    }

    smspdu_free_list( pdus );
}

extern "C" {
static void telephony_callback(void* userData, int numActiveCalls) {
    TelephonyPage* tpInst = (TelephonyPage*)userData;
    if (tpInst) {
        tpInst->eventLauncher(numActiveCalls);
    }
}
}

void TelephonyPage::setTelephonyAgent(const QAndroidTelephonyAgent* agent) {
    mTelephonyAgent = agent;

    if (mTelephonyAgent  &&  mTelephonyAgent->setNotifyCallback) {
        // Notify the agent that we want call-backs to tell us when the
        // telephone state changes
        mTelephonyAgent->setNotifyCallback(telephony_callback, (void*)this);
    }
}

void TelephonyPage::eventLauncher(int numActiveCalls) {
    QEvent* newTelephonyEvent = new TelephonyEvent(mCustomEventType, numActiveCalls);
    QCoreApplication::postEvent(this, newTelephonyEvent);
}

void TelephonyPage::customEvent(QEvent* cEvent) {
    assert(cEvent->type() == mCustomEventType);
    TelephonyEvent* tEvent = (TelephonyEvent*)cEvent;
    if (tEvent->numActiveCalls == 0) {
        if (mCallActivity != CallActivity::Inactive) {
            // The device has no calls but we're active.
            // They must have hung up on us. Push the
            // END CALL button on our side.
            on_tel_startEndButton_clicked();
        }
        // No 'else': If both we and the device have
        // no calls active, there is nothing to do.
//  } else {
//      // The device has a call active. If we are
//      // also active, there's nothing to do.
//      //
//      // If we are not active, the device is probably
//      // initiating a call. Maybe we should give the
//      // option to accept or reject that call.
//      //
//      // Note that the device can deal with multiple
//      // calls (e.g., call waiting), but our modem
//      // interface only tells us how many are active.
//      // We cannot tell which call was ended.
    }
}
