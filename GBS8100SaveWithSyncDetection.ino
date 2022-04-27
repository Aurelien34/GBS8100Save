#include <EEPROM.h>

// Constants

#define DEBOUNCING_DELAY	20
#define BUTTON_PUSH_DELAY 40
#define BUTTON_PUSH_DELAY_BETWEEN_PUSHES	40
#define VIDEO_SIGNAL_DETECTION_DELAY  2000
#define DELAY_GBS_ACTIVATION  2000
#define DELAY_GBS_RESET 1000
#define STARTUP_DELAY 2000
#define ROW_COUNT_DELAY  10
#define ROW_COUNT_TOLERANCE 40

#define PIN_RESET_NVRAM PIN_INPUT_MENU

#define PIN_DETECT_VIDEO_SIGNAL 2
#define PIN_ACTIVATE_GBS_NEG  A1

#define PIN_OUTPUT_BASE 8
#define PIN_OUTPUT_MENU (PIN_OUTPUT_BASE + 0)
#define PIN_OUTPUT_LEFT (PIN_OUTPUT_BASE + 1)
#define PIN_OUTPUT_RIGHT (PIN_OUTPUT_BASE + 2)
#define PIN_OUTPUT_UP (PIN_OUTPUT_BASE + 3)
#define PIN_OUTPUT_DOWN (PIN_OUTPUT_BASE + 4)

#define PIN_INPUT_BASE 3
#define PIN_INPUT_MENU (PIN_INPUT_BASE + 0)
#define PIN_INPUT_LEFT (PIN_INPUT_BASE + 1)
#define PIN_INPUT_RIGHT (PIN_INPUT_BASE + 2)
#define PIN_INPUT_UP (PIN_INPUT_BASE + 3)
#define PIN_INPUT_DOWN (PIN_INPUT_BASE + 4)

#define NVRAM_BASE	100
#define NVRAM_MAGIC_WORD_BASE	NVRAM_BASE
#define NVRAM_MAGIC_WORD_1 10
#define NVRAM_MAGIC_WORD_2 20
#define NVRAM_MAGIC_WORD_3 30
#define NVRAM_DATA_BASE (NVRAM_MAGIC_WORD_BASE + 3)

// Enumerations
typedef enum class menuParameter : byte
{
  Base = 0,
  NoMenu_Base = Base,

  NoMenu_HPos = (NoMenu_Base + 0),
  NoMenu_VPos = (NoMenu_Base + 1),

  NoMenu_LastItem = NoMenu_VPos,
  Menu_Base = 2,
  Menu_HSize = (Menu_Base + 0),
  Menu_VSize = (Menu_Base + 1),
  Menu_Brightness = (Menu_Base + 2),
  Menu_Contrast = (Menu_Base + 3),
  Menu_Hue = (Menu_Base + 4),
  Menu_Saturation = (Menu_Base + 5),
  Menu_Flick = (Menu_Base + 6),
  Menu_Sharpness = (Menu_Base + 7),
  Menu_OSDBackGround = (Menu_Base + 8),
  Menu_LastItem = Menu_OSDBackGround
} menuParameter_t;

typedef enum class button : byte
{
  buttonBase = 0,
  buttonMenu = (buttonBase + 0),
  buttonLeft = (buttonBase + 1),
  buttonRight = (buttonBase + 2),
  buttonUp = (buttonBase + 3),
  buttonDown = (buttonBase + 4),
  buttonLast = buttonDown,
  buttonNone = 0xFF
} button_t;

// Functions declaration

bool GetButtonState(button_t button);
button_t GetClickedButton();
bool UpdateParameters(button_t button);
bool UpdateParameter(menuParameter_t menuParam, bool increment);
byte GetNVRAMParameterValue(menuParameter_t menuParam);
void SetNVRAMParameterValue(menuParameter_t menuParam, byte value);
void UpdateNVRAM();
void ResetGBSState();
void ApplyNVRAMParameters();
void PushButton(button_t button);
//void BlinkDebug();

// Global variables
byte menuParameterPositionsMax[] =
{
  31, // NoMenu_HPos
  63, // NoMenu_VPos

  46, // Menu_HSize
  26, // Menu_VSize
  63, // Menu_Brightness
  63, // Menu_Contrast
  63, // Menu_Hue
  63, // Menu_Saturation
  63, // Menu_Flick
  63, // Menu_Sharpness
  2 // Menu_OSDBackGround
};

byte menuParameterValues[] =
{
  15, // NoMenu_HPos
  32, // NoMenu_VPos

  15, // Menu_HSize
  16, // Menu_VSize
  31, // Menu_Brightness
  32, // Menu_Contrast
  32, // Menu_Hue
  32, // Menu_Saturation
  32, // Menu_Flick
  32, // Menu_Sharpness
  0 // Menu_OSDBackGround
};

menuParameter_t currentMenuParam = menuParameter_t::Menu_OSDBackGround;

bool OSDMenuDisplayed = false;
int16_t rowCountCurrent = 0;
int16_t rowCountPrevious = 0;

// Functions definition

bool GetButtonState(button_t button)
{
  return !digitalRead(PIN_INPUT_BASE + ((byte)button) - (byte)button_t::buttonBase);
}

button_t GetClickedButton()
{
  // Iterate through buttons
  for (button_t button = button_t::buttonBase; button <= button_t::buttonLast; button = (button_t)((byte)button + 1))
  {
    // Has button been clicked?
    if (GetButtonState(button))
    {
      // Wait until the button is released
      do
      {
        delay(DEBOUNCING_DELAY);
      }
      while (GetButtonState(button));

      // This button has been clicked
      return button;
    }
  }

  // No button has been clicked
  return button_t::buttonNone;
}

bool UpdateParameters(button_t button)
{
  bool valueChanged = false;
  if (OSDMenuDisplayed)
  {
    // OSD Menu is displayed
    switch (button)
    {
      case button_t::buttonMenu:
        // Toggle menu
        OSDMenuDisplayed = false;
        break;
      case button_t::buttonUp:
        // Move the cursor in the menu
        if (currentMenuParam == menuParameter_t::Menu_Base)
          currentMenuParam = menuParameter_t::Menu_LastItem;
        else
          currentMenuParam = (menuParameter_t)((byte)currentMenuParam - 1);
        break;
      case button_t::buttonDown:
        // Move the cursor in the menu
        if (currentMenuParam == menuParameter_t::Menu_LastItem)
          currentMenuParam = menuParameter_t::Menu_Base;
        else
          currentMenuParam = (menuParameter_t)((byte)currentMenuParam + 1);
        break;
      case button_t::buttonLeft:
        // Update parameter value
        valueChanged = UpdateParameter(currentMenuParam, false);
        break;
      case button_t::buttonRight:
        // Update parameter value
        valueChanged = UpdateParameter(currentMenuParam, true);
        break;
    }
  }
  else
  {
    // OSD menu is not displayed
    switch (button)
    {
      case button_t::buttonMenu:
        // Toggle menu
        OSDMenuDisplayed = true;
        break;
      case button_t::buttonUp:
        valueChanged = UpdateParameter(menuParameter_t::NoMenu_VPos, false);
        break;
      case button_t::buttonDown:
        valueChanged = UpdateParameter(menuParameter_t::NoMenu_VPos, true);
        break;
      case button_t::buttonLeft:
        valueChanged = UpdateParameter(menuParameter_t::NoMenu_HPos, false);
        break;
      case button_t::buttonRight:
        valueChanged = UpdateParameter(menuParameter_t::NoMenu_HPos, true);
        break;
      default:
        valueChanged = false;
        break;
    }
  }
  return valueChanged;
}

bool UpdateParameter(menuParameter_t menuParam, bool increment)
{
  if (menuParam == menuParameter_t::Menu_OSDBackGround)
  {
    // Switch OSD param value (0 to 1, 1 to 0)
    menuParameterValues[(byte)menuParam] = ((menuParameterValues[(byte)menuParam] + 1) & 1);
    return true;
  }
  else if (increment)
  {
    // ++
    // Check bounds
    if (menuParameterValues[(byte)menuParam] < menuParameterPositionsMax[(byte)menuParam])
    {
      menuParameterValues[(byte)menuParam]++;
      return true;
    }
  }
  else
  {
    // --
    // Check bounds
    if (menuParameterValues[(byte)menuParam] > 0)
    {
      menuParameterValues[(byte)menuParam]--;
      return true;
    }
  }
  return false;
}

// Format NVRAM if it has never been used before
void PrepareNVRAM(bool forceClear)
{
  // Check magic word
  if (forceClear
      || EEPROM.read(NVRAM_MAGIC_WORD_BASE) != NVRAM_MAGIC_WORD_1
      || EEPROM.read(NVRAM_MAGIC_WORD_BASE + 1) != NVRAM_MAGIC_WORD_2
      || EEPROM.read(NVRAM_MAGIC_WORD_BASE + 2) != NVRAM_MAGIC_WORD_3)
  {
    // The magic word is not correct, need to clear the NVRAM
    // Write the default values
    for (byte i = 0; i < sizeof(menuParameterValues); ++i)
    {
      SetNVRAMParameterValue((menuParameter_t)i, menuParameterValues[i]);
    }

    // Write the magic word
    EEPROM.write(NVRAM_MAGIC_WORD_BASE, NVRAM_MAGIC_WORD_1);
    EEPROM.write(NVRAM_MAGIC_WORD_BASE + 1, NVRAM_MAGIC_WORD_2);
    EEPROM.write(NVRAM_MAGIC_WORD_BASE + 2, NVRAM_MAGIC_WORD_3);
  }
}

// Update parameters values in NVRAM
void UpdateNVRAM()
{
  // Compare NVRAM with parameter values one by one
  for (byte i = 0; i < sizeof(menuParameterValues); ++i)
  {
    menuParameter_t menuParam = (menuParameter_t)i;
    // Compare one parameter values
    if (GetNVRAMParameterValue(menuParam) != menuParameterValues[i])
    {
      // Update the parameter value in NVRAM
      SetNVRAMParameterValue(menuParam, menuParameterValues[i]);
    }
  }
}

byte GetNVRAMParameterValue(menuParameter_t menuParam)
{
  return EEPROM.read(NVRAM_DATA_BASE + (byte)menuParam);
}

void SetNVRAMParameterValue(menuParameter_t menuParam, byte value)
{
  EEPROM.write(NVRAM_DATA_BASE + (byte)menuParam, value);
}

void ResetGBSState()
{
  menuParameterValues[(int)menuParameter_t::NoMenu_HPos] = 15;
  menuParameterValues[(int)menuParameter_t::NoMenu_VPos] = 32;
  menuParameterValues[(int)menuParameter_t::Menu_HSize] = 15;
  menuParameterValues[(int)menuParameter_t::Menu_VSize] = 16;
  menuParameterValues[(int)menuParameter_t::Menu_Brightness] = 31;
  menuParameterValues[(int)menuParameter_t::Menu_Contrast] = 32;
  menuParameterValues[(int)menuParameter_t::Menu_Hue] = 32;
  menuParameterValues[(int)menuParameter_t::Menu_Saturation] = 32;
  menuParameterValues[(int)menuParameter_t::Menu_Flick] = 32;
  menuParameterValues[(int)menuParameter_t::Menu_Sharpness] = 32;
  menuParameterValues[(int)menuParameter_t::Menu_OSDBackGround] = 0;
}

void ApplyNVRAMParameters()
{
  // Iterate through Menu parameters and apply differences
  for (menuParameter_t menuParam = menuParameter_t::Base; (byte)menuParam <= (byte)menuParameter_t::Menu_LastItem; menuParam = (menuParameter_t)(((byte)menuParam) + 1))
  {
    // Handle menu / no menu parameters
    if (menuParam == menuParameter_t::Menu_Base)
    {
      // Show the menu
      PushButton(button_t::buttonMenu);
      // Switch to clear OSD mode
      PushButton(button_t::buttonUp);
      PushButton(button_t::buttonRight);
      PushButton(button_t::buttonDown);
    }

    if ((byte)menuParam >= (byte)menuParameter_t::Menu_Base)
    {
      // Go to the next menu item (also works for the first iteration, as the the default selected item is the last one in the list)
      PushButton(button_t::buttonDown);
    }

    // Apply the current parameter
    while (menuParameterValues[(byte)menuParam] != GetNVRAMParameterValue(menuParam))
    {
      if (menuParameterValues[(byte)menuParam] > GetNVRAMParameterValue(menuParam))
      {
        PushButton(menuParam == menuParameter_t::NoMenu_VPos ? button_t::buttonUp : button_t::buttonLeft);
        UpdateParameter(menuParam, false);
      }
      else
      {
        PushButton(menuParam == menuParameter_t::NoMenu_VPos ? button_t::buttonDown : button_t::buttonRight);
        UpdateParameter(menuParam, true);
      }
    }
  }

  // Hide the menu
  PushButton(button_t::buttonMenu);
}

void PushButton(button_t button)
{
  if ((byte)button >= (byte)button_t::buttonBase && (byte)button <= (byte)button_t::buttonLast)
  {
    byte buttonIndex = (byte)button - (byte)button_t::buttonBase;
    digitalWrite(PIN_OUTPUT_BASE + buttonIndex, HIGH);
    delay(BUTTON_PUSH_DELAY);
    digitalWrite(PIN_OUTPUT_BASE + buttonIndex, LOW);
    delay(BUTTON_PUSH_DELAY_BETWEEN_PUSHES);
  }
}

void WaitForVideoSignal()
{
  bool bDetected = false;
  while (!bDetected)
  {
    while (!digitalRead(PIN_DETECT_VIDEO_SIGNAL))
    {
      // Wait for signal to come
    }
    // A pulse was detected
    // Wait to ensure the signal is stable
    delay(VIDEO_SIGNAL_DETECTION_DELAY);
    // Do one more read...
    if (digitalRead(PIN_DETECT_VIDEO_SIGNAL))
    {
      // The signal is still there, it is considered stable
      bDetected = true;
    }
  }
}

void ActivateGBS(bool activate)
{
  digitalWrite(PIN_ACTIVATE_GBS_NEG, activate ? LOW : HIGH);
}

void ResetGBS()
{
  ActivateGBS(false);
  ResetGBSState();
  delay(DELAY_GBS_RESET);
  ActivateGBS(true);
}

void HSync()
{
  // Increment row count
  ++rowCountCurrent;
}

int16_t CountRowsPerInterval()
{
  // Disable interrupts (including the row counter) during row count assignment
  noInterrupts();
  // Clear row count
  rowCountCurrent = 0;
  // Enable interrupts and start counting
  interrupts();
  // Start counting period
  delay(ROW_COUNT_DELAY);
  // Disable interrupts (including the row counter) during row count reading
  noInterrupts();
  // Get row counter value
  int16_t newRowCount = rowCountCurrent;
  // Enable interrupts
  interrupts();
  // Return value
  return rowCountCurrent;
}

void setup()
{
  // Setup pins
  pinMode(PIN_ACTIVATE_GBS_NEG, OUTPUT);
  pinMode(PIN_RESET_NVRAM, INPUT_PULLUP);
  pinMode(PIN_DETECT_VIDEO_SIGNAL, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_DETECT_VIDEO_SIGNAL), HSync, FALLING);
  for (byte i = 0; i < 5; ++i)
  {
    pinMode(PIN_OUTPUT_BASE + i, OUTPUT);
    pinMode(PIN_INPUT_BASE + i, INPUT_PULLUP);
    digitalWrite(PIN_OUTPUT_BASE + i, LOW);
  }
  ResetGBSState();
  ActivateGBS(true);
  PrepareNVRAM(!digitalRead(PIN_RESET_NVRAM));

  // Wait for the system to stabilize
  delay(STARTUP_DELAY);
}

void loop()
{
  // Count rows per time interval
  int16_t newRowCount = CountRowsPerInterval();

  // Check if the row count difference is within the tolerance
  bool horizontalFrequencyChanged = (abs(newRowCount - rowCountPrevious) > ROW_COUNT_TOLERANCE);

  // Store row count for further comparison
  rowCountPrevious = newRowCount;

  // Did frequency change?
  if (horizontalFrequencyChanged)
  {
    // Do one more count to ensure the change did not happen during the measure and get the final measure
    rowCountPrevious = CountRowsPerInterval();
  }
  
  // Do we have a signal?
  if (rowCountPrevious > ROW_COUNT_TOLERANCE)
  {    
    // Video signal frequency changed?
    if (horizontalFrequencyChanged)
    {
      // Reset GBS
      ResetGBS();

      // Wait for GBS to turn on
      delay(DELAY_GBS_ACTIVATION);
  
      // Apply configuration
      ApplyNVRAMParameters();
    }

    // We have a video signal and the configuration is applied
    // Check buttons states
    button_t button = GetClickedButton();
    if (button != button_t::buttonNone)
    {
      // A button has been pressed, send it to the GBS keypad
      PushButton(button);
  
      // Check is the given button has an impact on the configuration and apply it
      if (UpdateParameters(button))
      {
        // Store new configuration to NVRAM
        UpdateNVRAM();
      }
    }
  }
}
