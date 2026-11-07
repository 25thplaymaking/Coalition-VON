//Menus to open
modded enum ChimeraMenuPreset
{
	ANPRC152,
	ANPRC150,
	ANPRC117,
	R168,
	R187
}

//What ear to hear the radio transmission in.
enum CVON_EStereo
{
	BOTH,
	RIGHT,
	LEFT
}

//Which screen the radio's display shows
enum CVON_ERadioScreen
{
	HOME,		//preset, net name and frequency
	ENTRY,		//typing a new frequency for the current preset (PGM, or CLR on the home screen)
	OPTIONS,	//OPT menu, scrolling the list
	EDIT		//OPT menu, changing one entry: ENT saves, CLR cancels
}

//The OPT menu's entries, in order
enum CVON_ERadioOption
{
	VOLUME,
	TX_POWER,		//transmit power LOW / MED / HIGH (CVON_RadioComponent.m_iTxPower)
	EAR,
	MONITOR,		//second channel heard at the same time (CVON_RadioComponent.m_iMonitorChannel)
	MONITOR_EAR,	//...and its ear
	LIGHT,			//display backlight
	CLOCK			//radio clock offset (CVON_RadioComponent.m_iTimeDeviation, minutes)
}

//Bifrost VON radio keypad and display. The keys do what their labels say on the real radio:
//- Harris radios (AN/PRC-152, -117, -150): PRE +/- step presets, VOL +/- volume, 7 OPT options menu,
//  8 PGM program the preset's frequency, 2 LT backlight, 6 and 9 are the up and down arrows inside menus,
//  CLR backs out without saving, ENT stores. The PRC-152 picture has no volume keys, so its left and right
//  arrows set the volume on the home screen.
//- R-168 and R-187: + / - (up and down arrows on the R-187) step channels, the side keys open the options,
//  the red and green keys clear and store, digits type a frequency, the volume knob or side keys set volume.
//Typing on the keyboard's number pad always enters a frequency, and Backspace works like CLR.
class CVON_RadioMenu: MenuBase
{
	static const string KEY_CLICK = "{E6153160DA26A03A}UI/sounds/RadioKnobClick.wav";
	static const float MESSAGE_TIME = 1.6;		//seconds a display message stays up
	static const float FLASH_TIME = 0.12;		//seconds a pressed key stays darkened
	static const float PENDING_TIME = 1.5;		//seconds a new preset/frequency is shown before the server confirms it
	static const float BLINK_TIME = 0.4;		//edit cursor blink
	static const int OPTION_COUNT = 7;
	static const int CLOCK_LIMIT = 99;			//clock offset range, minutes either way
	static const int LAST_CHANNEL = 99;
	static const int BAR_COUNT = 5;				//signal bars on the display

	//Backlight: one setting for every radio, like leaving the light switched on
	static bool s_bBacklight = false;

	Widget m_wRoot;

	//The radio we used to open the menu from the radial von menu (set by CVON_RadioComponent).
	IEntity m_RadioEntity;
	CVON_RadioComponent m_RadioComponent;
	SCR_PlayerController m_PlayerController;
	InputManager m_InputManager;
	protected bool m_bFocused = true;

	//Display: status line (left and right), net name line, big main line, info line (missing on small screens)
	TextWidget m_wStatus;
	TextWidget m_wStatusRight;
	TextWidget m_wName;
	TextWidget m_wMain;
	TextWidget m_wInfo;
	ImageWidget m_wGlow;
	ImageWidget m_wKnobWidget;

	//TX / RX marker and signal bars: lit bars use the display's ink, unlit ones a faint trace of it
	TextWidget m_wLink;
	ref array<ImageWidget> m_aBars = {};
	ref Color m_BarLit;
	ref Color m_BarDim;
	SCR_VONController m_VONController;

	Widget m_wHelpFrame;
	SCR_ButtonComponent m_wHelpButton;
	bool m_bHelpEnabled = false;

	//What the labelled keys do: Harris function keys, and whether the picture has volume keys
	bool m_bHarrisKeys;
	bool m_bVolumeKeys;

	CVON_ERadioScreen m_eScreen = CVON_ERadioScreen.HOME;
	CVON_ERadioOption m_eOption = CVON_ERadioOption.VOLUME;
	int m_iEditValue;
	string m_sEntry = "";

	string m_sMessage = "";
	float m_fMessageTimer;
	float m_fBlinkTimer;

	ButtonWidget m_wFlashed;
	float m_fFlashTimer;

	//Preset and frequency just chosen, shown until the server's copy catches up
	int m_iPendingChannel = -1;
	string m_sPendingFrequency = "";
	float m_fPendingTimer;

	//==========================================================================================================================================================================
	override void OnMenuOpen()
	{
		m_wRoot = GetRootWidget();
		m_InputManager = GetGame().GetInputManager();
		m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());

		m_InputManager.AddActionListener("Numpad1", EActionTrigger.DOWN, Numpad1);
		m_InputManager.AddActionListener("Numpad2", EActionTrigger.DOWN, Numpad2);
		m_InputManager.AddActionListener("Numpad3", EActionTrigger.DOWN, Numpad3);
		m_InputManager.AddActionListener("Numpad4", EActionTrigger.DOWN, Numpad4);
		m_InputManager.AddActionListener("Numpad5", EActionTrigger.DOWN, Numpad5);
		m_InputManager.AddActionListener("Numpad6", EActionTrigger.DOWN, Numpad6);
		m_InputManager.AddActionListener("Numpad7", EActionTrigger.DOWN, Numpad7);
		m_InputManager.AddActionListener("Numpad8", EActionTrigger.DOWN, Numpad8);
		m_InputManager.AddActionListener("Numpad9", EActionTrigger.DOWN, Numpad9);
		m_InputManager.AddActionListener("Numpad0", EActionTrigger.DOWN, Numpad0);
		m_InputManager.AddActionListener("Backspace", EActionTrigger.DOWN, Backspace);
		m_InputManager.AddActionListener("VONMenu", EActionTrigger.DOWN, Close);

		array<string> keys = {"Button0", "Button1", "Button2", "Button3", "Button4", "Button5", "Button6", "Button7", "Button8", "Button9",
			"Clear", "Enter", "MenuLeft", "MenuRight", "MenuUp", "MenuDown", "VolUp", "VolDown", "Power"};
		foreach (string key: keys)
		{
			Widget keyWidget = m_wRoot.FindAnyWidget(key);
			if (!keyWidget)
				continue;
			SCR_ButtonBaseComponent button = SCR_ButtonBaseComponent.Cast(keyWidget.FindHandler(SCR_ButtonBaseComponent));
			if (button)
				button.m_OnClicked.Insert(OnKeyClicked);
		}
		m_bVolumeKeys = m_wRoot.FindAnyWidget("VolUp") != null;

		m_wStatus = TextWidget.Cast(m_wRoot.FindAnyWidget("LcdStatus"));
		m_wStatusRight = TextWidget.Cast(m_wRoot.FindAnyWidget("LcdStatusRight"));
		m_wName = TextWidget.Cast(m_wRoot.FindAnyWidget("LcdName"));
		m_wMain = TextWidget.Cast(m_wRoot.FindAnyWidget("LcdMain"));
		m_wInfo = TextWidget.Cast(m_wRoot.FindAnyWidget("LcdInfo"));
		m_wGlow = ImageWidget.Cast(m_wRoot.FindAnyWidget("LcdGlow"));
		m_wKnobWidget = ImageWidget.Cast(m_wRoot.FindAnyWidget("Knob"));

		m_wLink = TextWidget.Cast(m_wRoot.FindAnyWidget("LcdLink"));
		for (int bar = 0; bar < BAR_COUNT; bar++)
		{
			ImageWidget barWidget = ImageWidget.Cast(m_wRoot.FindAnyWidget("LcdBar" + bar.ToString()));
			if (barWidget)
				m_aBars.Insert(barWidget);
		}
		if (!m_aBars.IsEmpty())
		{
			Color ink = m_aBars[0].GetColor();
			m_BarLit = new Color(ink.R(), ink.G(), ink.B(), ink.A());
			//Dark ink is an LCD, whose unlit segments stay faintly visible; bright ink is a glowing display
			//(the R-168), where they almost vanish
			float unlit = 0.14;
			if (ink.R() + ink.G() + ink.B() > 1.5)
				unlit = 0.06;
			m_BarDim = new Color(ink.R(), ink.G(), ink.B(), ink.A() * unlit);
		}
		if (m_PlayerController)
			m_VONController = SCR_VONController.Cast(m_PlayerController.FindComponent(SCR_VONController));

		Widget helpButton = m_wRoot.FindAnyWidget("HelpButton");
		if (helpButton)
		{
			m_wHelpButton = SCR_ButtonComponent.Cast(helpButton.FindHandler(SCR_ButtonComponent));
			if (m_wHelpButton)
				m_wHelpButton.m_OnClicked.Insert(ToggleHelp);
		}
		m_wHelpFrame = m_wRoot.FindAnyWidget("HelpFrame");
	}

	void ToggleHelp()
	{
		m_bHelpEnabled = !m_bHelpEnabled;
	}

	//==========================================================================================================================================================================
	override void OnMenuClose()
	{
		m_InputManager.RemoveActionListener("Numpad1", EActionTrigger.DOWN, Numpad1);
		m_InputManager.RemoveActionListener("Numpad2", EActionTrigger.DOWN, Numpad2);
		m_InputManager.RemoveActionListener("Numpad3", EActionTrigger.DOWN, Numpad3);
		m_InputManager.RemoveActionListener("Numpad4", EActionTrigger.DOWN, Numpad4);
		m_InputManager.RemoveActionListener("Numpad5", EActionTrigger.DOWN, Numpad5);
		m_InputManager.RemoveActionListener("Numpad6", EActionTrigger.DOWN, Numpad6);
		m_InputManager.RemoveActionListener("Numpad7", EActionTrigger.DOWN, Numpad7);
		m_InputManager.RemoveActionListener("Numpad8", EActionTrigger.DOWN, Numpad8);
		m_InputManager.RemoveActionListener("Numpad9", EActionTrigger.DOWN, Numpad9);
		m_InputManager.RemoveActionListener("Numpad0", EActionTrigger.DOWN, Numpad0);
		m_InputManager.RemoveActionListener("Backspace", EActionTrigger.DOWN, Backspace);
		m_InputManager.RemoveActionListener("VONMenu", EActionTrigger.DOWN, Close);
	}

	//==========================================================================================================================================================================
	override void OnMenuUpdate(float tDelta)
	{
		if (m_wHelpFrame)
			m_wHelpFrame.SetVisible(m_bHelpEnabled);

		if (!m_RadioComponent)
		{
			if (!m_RadioEntity)
				return;
			m_RadioComponent = CVON_RadioComponent.Cast(m_RadioEntity.FindComponent(CVON_RadioComponent));
			if (!m_RadioComponent)
				return;
			m_bHarrisKeys = IsHarrisRadio(m_RadioComponent.m_eRadioMenu);
			UpdateKnob();
		}

		if (m_fMessageTimer > 0)
		{
			m_fMessageTimer -= tDelta;
			if (m_fMessageTimer <= 0)
				m_sMessage = "";
		}

		if (m_fFlashTimer > 0)
		{
			m_fFlashTimer -= tDelta;
			if (m_fFlashTimer <= 0)
				EndFlash();
		}

		if (m_fPendingTimer > 0)
		{
			m_fPendingTimer -= tDelta;
			bool caughtUp = m_RadioComponent.m_iCurrentChannel == m_iPendingChannel && m_RadioComponent.m_sFrequency == m_sPendingFrequency;
			if (m_fPendingTimer <= 0 || caughtUp)
				ClearPending();
		}

		m_fBlinkTimer += tDelta;
		if (m_fBlinkTimer >= BLINK_TIME * 2)
			m_fBlinkTimer = 0;

		RefreshDisplay();
	}

	static bool IsHarrisRadio(int menuPreset)
	{
		return menuPreset == ChimeraMenuPreset.ANPRC152 || menuPreset == ChimeraMenuPreset.ANPRC117 || menuPreset == ChimeraMenuPreset.ANPRC150;
	}

	//Keys
	//==========================================================================================================================================================================
	void OnKeyClicked(SCR_ButtonBaseComponent button)
	{
		if (!button || !button.GetRootWidget())
			return;
		PressKey(button.GetRootWidget().GetName(), false);
	}

	void Numpad0() { PressKey("Button0", true); }
	void Numpad1() { PressKey("Button1", true); }
	void Numpad2() { PressKey("Button2", true); }
	void Numpad3() { PressKey("Button3", true); }
	void Numpad4() { PressKey("Button4", true); }
	void Numpad5() { PressKey("Button5", true); }
	void Numpad6() { PressKey("Button6", true); }
	void Numpad7() { PressKey("Button7", true); }
	void Numpad8() { PressKey("Button8", true); }
	void Numpad9() { PressKey("Button9", true); }
	void Backspace() { PressKey("Clear", true); }

	//"Button7" -> 7, anything else -> -1
	static int DigitOfKey(string key)
	{
		if (key.Length() != 7 || key.IndexOf("Button") != 0)
			return -1;
		return key.Substring(6, 1).ToInt();
	}

	//Every key press lands here. fromKeyboard: the number pad or Backspace rather than a click on the radio.
	void PressKey(string key, bool fromKeyboard)
	{
		if (!m_RadioComponent)
			return;

		Flash(key);
		if (key == "Power")
		{
			TogglePower();
			return;
		}
		if (!m_RadioComponent.m_bPower)
			return;

		AudioSystem.PlaySound(KEY_CLICK);
		//The next key press clears a message (on small screens it covers the name line)
		m_sMessage = "";
		m_fMessageTimer = 0;

		if (key == "VolUp" || key == "VolDown")
		{
			if (key == "VolUp")
				StepVolume(1);
			else
				StepVolume(-1);
			return;
		}

		int digit = DigitOfKey(key);
		switch (m_eScreen)
		{
			case CVON_ERadioScreen.HOME: {HomeKey(key, digit, fromKeyboard); break;}
			case CVON_ERadioScreen.ENTRY: {EntryKey(key, digit); break;}
			case CVON_ERadioScreen.OPTIONS: {OptionsKey(key, digit); break;}
			case CVON_ERadioScreen.EDIT: {EditKey(key, digit); break;}
		}
		RefreshDisplay();
	}

	//Home screen
	//==========================================================================================================================================================================
	void HomeKey(string key, int digit, bool fromKeyboard)
	{
		if (key == "MenuUp")
		{
			StepPreset(1);
			return;
		}
		if (key == "MenuDown")
		{
			StepPreset(-1);
			return;
		}
		if (key == "MenuLeft" || key == "MenuRight")
		{
			int direction = 1;
			if (key == "MenuLeft")
				direction = -1;
			//No volume keys in the picture (PRC-152): the arrows set the volume
			if (!m_bVolumeKeys)
			{
				StepVolume(direction);
				return;
			}
			OpenOptions();
			if (direction < 0)
				m_eOption = OPTION_COUNT - 1;
			return;
		}
		if (key == "Clear")
		{
			OpenEntry("");
			return;
		}
		if (digit < 0)
			return;

		//Harris keypads: on the home screen the digit keys are their function labels
		if (m_bHarrisKeys && !fromKeyboard)
		{
			switch (digit)
			{
				case 2:
				{
					s_bBacklight = !s_bBacklight;
					if (s_bBacklight)
						ShowMessage("LIGHT ON");
					else
						ShowMessage("LIGHT OFF");
					return;
				}
				case 7: {OpenOptions(); return;}
				case 8: {OpenEntry(""); return;}
			}
			ShowMessage("NOT AVAILABLE");
			return;
		}
		OpenEntry(digit.ToString());
	}

	//Frequency entry
	//==========================================================================================================================================================================
	void OpenEntry(string firstDigit)
	{
		m_eScreen = CVON_ERadioScreen.ENTRY;
		m_sEntry = firstDigit;
		m_sMessage = "";
	}

	int MaxDigits()
	{
		return Math.Max(m_RadioComponent.m_iMaxFrequency.ToString().Length(), 5);
	}

	void EntryKey(string key, int digit)
	{
		if (digit >= 0)
		{
			if (m_sEntry.Length() < MaxDigits())
				m_sEntry += digit.ToString();
			return;
		}
		if (key == "Clear")
		{
			if (m_sEntry.IsEmpty())
				m_eScreen = CVON_ERadioScreen.HOME;
			else
				m_sEntry = m_sEntry.Substring(0, m_sEntry.Length() - 1);
			return;
		}
		if (key == "Enter")
		{
			StoreEntry();
			return;
		}
		//PRE works from anywhere: leave the entry and change preset
		if (key == "MenuUp" || key == "MenuDown")
		{
			m_eScreen = CVON_ERadioScreen.HOME;
			if (key == "MenuUp")
				StepPreset(1);
			else
				StepPreset(-1);
		}
	}

	void StoreEntry()
	{
		int frequency = m_sEntry.ToInt();
		if (m_sEntry.IsEmpty() || !m_RadioComponent.IsValidFrequency(frequency))
		{
			ShowMessage("INVALID FREQ");
			m_sEntry = "";
			return;
		}
		string text = frequency.ToString();
		m_RadioComponent.UpdateFrequencyClient(text);
		SetPending(ShownChannel(), text);
		m_eScreen = CVON_ERadioScreen.HOME;
		ShowMessage("STORED");
	}

	//OPT menu
	//==========================================================================================================================================================================
	void OpenOptions()
	{
		m_eScreen = CVON_ERadioScreen.OPTIONS;
		m_sMessage = "";
	}

	//Up: PRE+ / up arrow / Harris 6. Down: PRE- / down arrow / Harris 9. Left and right arrows too.
	int MenuDirection(string key, int digit)
	{
		if (key == "MenuUp" || key == "MenuRight" || (m_bHarrisKeys && digit == 6))
			return 1;
		if (key == "MenuDown" || key == "MenuLeft" || (m_bHarrisKeys && digit == 9))
			return -1;
		return 0;
	}

	void OptionsKey(string key, int digit)
	{
		if (key == "Clear")
		{
			m_eScreen = CVON_ERadioScreen.HOME;
			return;
		}
		if (key == "Enter")
		{
			m_iEditValue = OptionValue(m_eOption);
			m_eScreen = CVON_ERadioScreen.EDIT;
			m_fBlinkTimer = 0;
			return;
		}
		int direction = MenuDirection(key, digit);
		//Scrolling down the list goes to the next entry
		if (key == "MenuUp" || key == "MenuDown" || (m_bHarrisKeys && (digit == 6 || digit == 9)))
			direction = -direction;
		if (direction == 0)
			return;
		int option = (m_eOption + direction) % OPTION_COUNT;
		if (option < 0)
			option += OPTION_COUNT;
		m_eOption = option;
	}

	void EditKey(string key, int digit)
	{
		if (key == "Clear")
		{
			m_eScreen = CVON_ERadioScreen.OPTIONS;
			return;
		}
		if (key == "Enter")
		{
			ApplyOption(m_eOption, m_iEditValue);
			m_eScreen = CVON_ERadioScreen.OPTIONS;
			ShowMessage("SAVED");
			return;
		}
		int direction = MenuDirection(key, digit);
		if (direction != 0)
		{
			m_iEditValue = NextOptionValue(m_eOption, m_iEditValue, direction);
			m_fBlinkTimer = 0;
		}
	}

	int OptionValue(CVON_ERadioOption option)
	{
		switch (option)
		{
			case CVON_ERadioOption.VOLUME: {return m_RadioComponent.m_iVolume;}
			case CVON_ERadioOption.TX_POWER: {return m_RadioComponent.m_iTxPower;}
			case CVON_ERadioOption.EAR: {return m_RadioComponent.m_eStereo;}
			case CVON_ERadioOption.MONITOR: {return m_RadioComponent.m_iMonitorChannel;}
			case CVON_ERadioOption.MONITOR_EAR: {return m_RadioComponent.m_eMonitorStereo;}
			case CVON_ERadioOption.LIGHT:
			{
				if (s_bBacklight)
					return 1;
				return 0;
			}
			case CVON_ERadioOption.CLOCK: {return m_RadioComponent.m_iTimeDeviation;}
		}
		return 0;
	}

	int NextOptionValue(CVON_ERadioOption option, int value, int direction)
	{
		switch (option)
		{
			case CVON_ERadioOption.VOLUME: {return Math.ClampInt(value + direction, 0, 9);}
			case CVON_ERadioOption.TX_POWER: {return Math.ClampInt(value + direction, CVON_ETxPower.LOW, CVON_ETxPower.HIGH);}
			case CVON_ERadioOption.EAR: {return NextStereo(value, direction);}
			case CVON_ERadioOption.MONITOR: {return NextMonitorChannel(value, direction);}
			case CVON_ERadioOption.MONITOR_EAR: {return NextStereo(value, direction);}
			case CVON_ERadioOption.LIGHT: {return 1 - value;}
			case CVON_ERadioOption.CLOCK: {return Math.ClampInt(value + direction, -CLOCK_LIMIT, CLOCK_LIMIT);}
		}
		return value;
	}

	//Monitor: Off, then each channel except the one the radio is on
	int NextMonitorChannel(int monitor, int direction)
	{
		int channelCount = m_RadioComponent.m_aChannels.Count();
		if (channelCount < 2)
			return 0;
		for (int i = 0; i <= channelCount; i++)
		{
			monitor = (monitor + direction) % (channelCount + 1);
			if (monitor < 0)
				monitor += channelCount + 1;
			if (monitor != ShownChannel())
				break;
		}
		return monitor;
	}

	void ApplyOption(CVON_ERadioOption option, int value)
	{
		switch (option)
		{
			case CVON_ERadioOption.VOLUME: {SetVolume(value); return;}
			case CVON_ERadioOption.TX_POWER: {m_RadioComponent.SetTxPowerClient(value); return;}
			case CVON_ERadioOption.EAR:
			{
				m_RadioComponent.m_eStereo = value;
				SaveSettings();
				return;
			}
			case CVON_ERadioOption.MONITOR: {m_RadioComponent.SetMonitorChannelClient(value); return;}
			case CVON_ERadioOption.MONITOR_EAR:
			{
				m_RadioComponent.m_eMonitorStereo = value;
				SaveSettings();
				return;
			}
			case CVON_ERadioOption.LIGHT: {s_bBacklight = value == 1; return;}
			case CVON_ERadioOption.CLOCK:
			{
				m_RadioComponent.m_iTimeDeviation = value;
				m_RadioComponent.UpdateTimeDeviationClient(value);
				return;
			}
		}
	}

	static string OptionTitle(CVON_ERadioOption option)
	{
		switch (option)
		{
			case CVON_ERadioOption.VOLUME: {return "VOLUME";}
			case CVON_ERadioOption.TX_POWER: {return "TX POWER";}
			case CVON_ERadioOption.EAR: {return "EAR";}
			case CVON_ERadioOption.MONITOR: {return "MONITOR";}
			case CVON_ERadioOption.MONITOR_EAR: {return "MONITOR EAR";}
			case CVON_ERadioOption.LIGHT: {return "LIGHT";}
			case CVON_ERadioOption.CLOCK: {return "CLOCK";}
		}
		return "";
	}

	string OptionText(CVON_ERadioOption option, int value)
	{
		switch (option)
		{
			case CVON_ERadioOption.VOLUME: {return value.ToString();}
			case CVON_ERadioOption.TX_POWER: {return CVON_RadioComponent.TxPowerText(value) + " " + RangeText(PowerRange(value));}
			case CVON_ERadioOption.EAR: {return EarText(value);}
			case CVON_ERadioOption.MONITOR:
			{
				if (value < 1 || value > m_RadioComponent.m_aChannels.Count())
					return "OFF";
				return "P" + PadChannel(value) + " " + FormatFrequency(m_RadioComponent.m_aChannels.Get(value - 1));
			}
			case CVON_ERadioOption.MONITOR_EAR: {return EarText(value);}
			case CVON_ERadioOption.LIGHT:
			{
				if (value == 1)
					return "ON";
				return "OFF";
			}
			case CVON_ERadioOption.CLOCK: {return ClockText(value);}
		}
		return "";
	}

	//Metres the radio would reach on this power setting
	float PowerRange(int power)
	{
		switch (power)
		{
			case CVON_ETxPower.LOW: {return m_RadioComponent.m_iRadioRange * m_RadioComponent.m_fLowPowerRange;}
			case CVON_ETxPower.MEDIUM: {return m_RadioComponent.m_iRadioRange * m_RadioComponent.m_fMediumPowerRange;}
		}
		return m_RadioComponent.m_iRadioRange;
	}

	//1250 -> "1.3KM", 15000 -> "15KM"
	static string RangeText(float metres)
	{
		int tenths = Math.Round(metres / 100);
		int whole = tenths / 10;
		if (tenths >= 100)
			return whole.ToString() + "KM";
		int fraction = tenths % 10;
		return whole.ToString() + "." + fraction.ToString() + "KM";
	}

	//"L", "M" or "H", as the Harris displays show the power setting
	static string PowerLetter(int power)
	{
		switch (power)
		{
			case CVON_ETxPower.LOW: {return "L";}
			case CVON_ETxPower.MEDIUM: {return "M";}
		}
		return "H";
	}

	//Radio time with an offset in minutes, HH:MM
	static string ClockText(int offsetMinutes)
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world || !world.GetTimeAndWeatherManager())
			return "--:--";
		int h, m, s;
		world.GetTimeAndWeatherManager().GetHoursMinutesSeconds(h, m, s);
		int minutes = (h * 60 + m + offsetMinutes) % 1440;
		if (minutes < 0)
			minutes += 1440;
		return PadChannel(minutes / 60) + ":" + PadChannel(minutes % 60);
	}

	//Presets, volume, settings
	//==========================================================================================================================================================================
	int ShownChannel()
	{
		if (m_iPendingChannel > 0)
			return m_iPendingChannel;
		return m_RadioComponent.m_iCurrentChannel;
	}

	string ShownFrequency()
	{
		if (m_sPendingFrequency != "")
			return m_sPendingFrequency;
		int channel = ShownChannel();
		if (channel == m_RadioComponent.m_iCurrentChannel)
			return m_RadioComponent.m_sFrequency;
		if (channel >= 1 && channel <= m_RadioComponent.m_aChannels.Count())
			return m_RadioComponent.m_aChannels.Get(channel - 1);
		return "55500";
	}

	void SetPending(int channel, string frequency)
	{
		m_iPendingChannel = channel;
		m_sPendingFrequency = frequency;
		m_fPendingTimer = PENDING_TIME;
	}

	void ClearPending()
	{
		m_iPendingChannel = -1;
		m_sPendingFrequency = "";
		m_fPendingTimer = 0;
	}

	//PRE +/-: the next preset; stepping past the last one makes a new one (up to 99), PRE- from 1 wraps to the last
	void StepPreset(int direction)
	{
		int channelCount = m_RadioComponent.m_aChannels.Count();
		int next = ShownChannel() + direction;
		if (next > LAST_CHANNEL)
			next = 1;
		if (next < 1)
			next = Math.Max(channelCount, 1);

		string frequency;
		if (next > channelCount)
		{
			m_RadioComponent.AddChannelClient();
			frequency = "55500";
		}
		else
			frequency = m_RadioComponent.m_aChannels.Get(next - 1);

		m_RadioComponent.UpdateChannelClient(next);
		m_RadioComponent.UpdateFrequencyClient(frequency);
		SetPending(next, frequency);
		//A monitor on the channel we just moved to would hear the same net twice
		if (m_RadioComponent.m_iMonitorChannel == next)
			m_RadioComponent.SetMonitorChannelClient(0);
	}

	void StepVolume(int direction)
	{
		SetVolume(Math.ClampInt(m_RadioComponent.m_iVolume + direction, 0, 9));
		ShowMessage("VOLUME " + m_RadioComponent.m_iVolume.ToString());
	}

	void SetVolume(int volume)
	{
		m_RadioComponent.m_iVolume = volume;
		SaveSettings();
	}

	void SaveSettings()
	{
		m_RadioComponent.WriteJSON(SCR_PlayerController.GetLocalControlledEntity());
		if (m_PlayerController)
			m_PlayerController.UpdateSettings();
	}

	void ShowMessage(string message)
	{
		m_sMessage = message;
		m_fMessageTimer = MESSAGE_TIME;
	}

	//Power
	//==========================================================================================================================================================================
	void TogglePower()
	{
		#ifdef WORKBENCH
		m_RadioComponent.m_bPower = !m_RadioComponent.m_bPower;
		#else
		//A client shows the change at once and the server's copy, flipped by the request, replicates back the same
		//value. On a hosting player's machine the request flips it directly, so flipping here as well (as the
		//original did) cancelled the switch out.
		if (!Replication.IsServer())
			m_RadioComponent.m_bPower = !m_RadioComponent.m_bPower;
		m_RadioComponent.TogglePowerClient();
		#endif
		m_eScreen = CVON_ERadioScreen.HOME;
		m_sMessage = "";
		UpdateKnob();
		RefreshDisplay();
		AudioSystem.PlaySound(KEY_CLICK);
	}

	void UpdateKnob()
	{
		if (!m_wKnobWidget)
			return;
		if (m_RadioComponent.m_bPower)
			m_wKnobWidget.LoadImageTexture(1, m_RadioComponent.m_sDialOn);
		else
			m_wKnobWidget.LoadImageTexture(1, m_RadioComponent.m_sDialOff);
		m_wKnobWidget.SetImage(1);
	}

	//Pressed key: darkened for a moment
	//==========================================================================================================================================================================
	void Flash(string key)
	{
		EndFlash();
		m_wFlashed = ButtonWidget.Cast(m_wRoot.FindAnyWidget(key));
		if (!m_wFlashed)
			return;
		m_wFlashed.SetColor(Color.FromRGBA(0, 0, 0, 110));
		m_fFlashTimer = FLASH_TIME;
	}

	void EndFlash()
	{
		if (m_wFlashed)
			m_wFlashed.SetColor(Color.FromRGBA(255, 255, 255, 0));
		m_wFlashed = null;
		m_fFlashTimer = 0;
	}

	//Display
	//==========================================================================================================================================================================
	void RefreshDisplay()
	{
		if (!m_RadioComponent)
			return;

		bool on = m_RadioComponent.m_bPower;
		if (m_wGlow)
			m_wGlow.SetVisible(on && s_bBacklight);
		SetLine(m_wStatus, on, "");
		SetLine(m_wStatusRight, on, "");
		SetLine(m_wName, on, "");
		SetLine(m_wMain, on, "");
		SetLine(m_wInfo, on, "");
		if (!on)
		{
			RefreshLink(false);
			return;
		}

		string status, statusRight, name, mainText, info;
		switch (m_eScreen)
		{
			case CVON_ERadioScreen.HOME:
			{
				int channel = ShownChannel();
				string frequency = ShownFrequency();
				status = "P" + PadChannel(channel) + "  " + CryptoText() + "  " + PowerLetter(m_RadioComponent.m_iTxPower);
				statusRight = "VOL " + m_RadioComponent.m_iVolume.ToString();
				name = m_RadioComponent.GetChannelLabel(frequency);
				if (name == "")
					name = "PRESET " + PadChannel(channel);
				mainText = FormatFrequency(frequency);
				string monitor = m_RadioComponent.GetMonitorFrequency();
				if (monitor != "")
					info = "MON P" + PadChannel(m_RadioComponent.m_iMonitorChannel) + " " + FormatFrequency(monitor) + " " + EarLetter(m_RadioComponent.m_eMonitorStereo);
				else
					info = "EAR " + EarText(m_RadioComponent.m_eStereo);
				break;
			}
			case CVON_ERadioScreen.ENTRY:
			{
				status = "PGM P" + PadChannel(ShownChannel());
				name = "ENTER FREQ";
				mainText = FormatFrequency(EntryMask());
				info = "ENT STORE  CLR DEL";
				break;
			}
			case CVON_ERadioScreen.OPTIONS:
			{
				status = OptionsStatus();
				name = OptionTitle(m_eOption);
				mainText = OptionText(m_eOption, OptionValue(m_eOption));
				info = "ENT EDIT  CLR EXIT";
				break;
			}
			case CVON_ERadioScreen.EDIT:
			{
				status = OptionsStatus();
				statusRight = "EDIT";
				name = OptionTitle(m_eOption);
				//The value being changed blinks, like the real radio's edit field
				if (m_fBlinkTimer < BLINK_TIME)
					mainText = OptionText(m_eOption, m_iEditValue);
				info = "ENT SAVE  CLR CANCEL";
				break;
			}
		}

		//Messages take the info line, or the name line on screens too small for one
		if (m_sMessage != "")
		{
			if (m_wInfo)
				info = m_sMessage;
			else
				name = m_sMessage;
		}

		SetLine(m_wStatus, true, status);
		SetLine(m_wStatusRight, true, statusRight);
		SetLine(m_wName, true, name);
		SetLine(m_wMain, true, mainText);
		SetLine(m_wInfo, true, info);
		RefreshLink(true);
	}

	//T while this radio transmits (bars = power setting), R while it hears a call (bars = signal strength)
	void RefreshLink(bool on)
	{
		bool transmitting;
		float quality;
		bool active = on && m_VONController && m_VONController.GetRadioLink(m_RadioComponent, transmitting, quality);
		string marker = "";
		int lit = 0;
		if (active)
		{
			if (transmitting)
			{
				marker = "T";
				lit = TxPowerBars(m_RadioComponent.m_iTxPower);
			}
			else
			{
				marker = "R";
				lit = SignalBars(quality);
			}
		}
		SetLine(m_wLink, on, marker);
		for (int bar = 0; bar < m_aBars.Count(); bar++)
		{
			m_aBars[bar].SetVisible(on);
			if (bar < lit)
				m_aBars[bar].SetColor(m_BarLit);
			else
				m_aBars[bar].SetColor(m_BarDim);
		}
	}

	//Signal strength 0..1 -> lit bars (any signal at all lights one)
	static int SignalBars(float quality)
	{
		if (quality <= 0)
			return 0;
		int bars = Math.Ceil(quality * BAR_COUNT);
		return Math.ClampInt(bars, 1, BAR_COUNT);
	}

	//While transmitting the bars show the power setting
	static int TxPowerBars(int power)
	{
		switch (power)
		{
			case CVON_ETxPower.LOW: {return 2;}
			case CVON_ETxPower.MEDIUM: {return 3;}
		}
		return BAR_COUNT;
	}

	//"OPT 2/6"
	string OptionsStatus()
	{
		int number = m_eOption + 1;
		int count = OPTION_COUNT;
		return "OPT " + number.ToString() + "/" + count.ToString();
	}

	static void SetLine(TextWidget line, bool visible, string text)
	{
		if (!line)
			return;
		line.SetVisible(visible);
		line.SetText(text);
	}

	//Digits typed so far, padded with underscores to a full frequency
	string EntryMask()
	{
		string mask = m_sEntry;
		while (mask.Length() < MaxDigits())
			mask += "_";
		return mask;
	}

	//CT when the radio holds a crypto key (faction encryption on), PT (plain text) otherwise
	string CryptoText()
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gamemode && gamemode.IsEncryptionEnabled() && m_RadioComponent.m_sFactionKey != "")
			return "CT";
		return "PT";
	}

	//"31120" (kHz) -> "31.120" (MHz), the way the radios show it
	static string FormatFrequency(string frequency)
	{
		int length = frequency.Length();
		if (length < 4)
			return frequency;
		return frequency.Substring(0, length - 3) + "." + frequency.Substring(length - 3, 3);
	}

	static string PadChannel(int channel)
	{
		if (channel >= 0 && channel < 10)
			return "0" + channel.ToString();
		return channel.ToString();
	}

	static string EarLetter(CVON_EStereo stereo)
	{
		switch (stereo)
		{
			case CVON_EStereo.RIGHT: {return "R";}
			case CVON_EStereo.LEFT: {return "L";}
		}
		return "LR";
	}

	//Both, Right, Left, round and round.
	static CVON_EStereo NextStereo(CVON_EStereo current, int input)
	{
		int next = (current + input) % 3;
		if (next < 0)
			next += 3;
		return next;
	}

	//The ear as the display shows it
	static string EarText(CVON_EStereo stereo)
	{
		switch (stereo)
		{
			case CVON_EStereo.RIGHT: {return "RIGHT";}
			case CVON_EStereo.LEFT: {return "LEFT";}
		}
		return "BOTH";
	}

	static string StereoText(CVON_EStereo stereo)
	{
		switch (stereo)
		{
			case CVON_EStereo.RIGHT: {return "Right";}
			case CVON_EStereo.LEFT: {return "Left";}
		}
		return "Both";
	}

	//==========================================================================================================================================================================
	override void OnMenuFocusLost()
	{
		m_bFocused = false;
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
		#endif
	}

	//==========================================================================================================================================================================
	override void OnMenuFocusGained()
	{
		m_bFocused = true;
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
		#endif
	}
}
