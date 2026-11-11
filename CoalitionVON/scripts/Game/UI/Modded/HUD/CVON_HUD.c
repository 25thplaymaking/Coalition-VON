class CVON_HUD: SCR_InfoDisplay {
	
	protected SCR_PlayerController m_PlayerController;
	protected CVON_VONGameModeComponent m_VONGamemode;
	protected ProgressBarWidget m_wVoiceRangeSlider;
	protected TextWidget m_wVoiceRangeText;
	protected InputManager m_InputManager;
	bool m_bIsToggled = false;
	
	const static ResourceName m_sArrowOff = "{D29A20E07AC57BFA}UI/images/Indicators/VON_arrowOFF.edds";
	const static ResourceName m_sArrowOn = "{DFDD89B0F30E34C9}UI/images/Indicators/VON_arrowON.edds";
	const static ResourceName m_sButtonOff = "{E35AF2B54C7D8F43}UI/images/Indicators/VON_buttonoff.edds";
	const static ResourceName m_sButtonOn = "{A9118B8FE72B0F25}UI/images/Indicators/VON_buttonon.edds";
	
	const static ResourceName m_sMicOff = "{A7ADCCC7757DB6A1}UI/images/Mic/mic_off.edds";
	const static ResourceName m_sMicVol1 = "{BECAFDAE3D934EDF}UI/images/Mic/mic_vol1.edds";
	const static ResourceName m_sMicVol2 = "{E022D4ED821EC0D0}UI/images/Mic/mic_vol2.edds";
	const static ResourceName m_sMicVol3 = "{142A93758FC3A8A4}UI/images/Mic/mic_vol3.edds";
	const static ResourceName m_sMicVol4 = "{5DF2866AFD05DCCE}UI/images/Mic/mic_vol4.edds";
	const static ResourceName m_sMicVol5 = "{A9FAC1F2F0D8B4BA}UI/images/Mic/mic_vol5.edds"; 


	//------------------------------------------------------------------------------------------------

	// Override/static functions

	//------------------------------------------------------------------------------------------------
	
	override event void OnStartDraw(IEntity owner)
	{
		super.OnStartDraw(owner);
		if (!CVON_VONGameModeComponent.GetInstance())
			return;
		m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		m_VONGamemode = CVON_VONGameModeComponent.GetInstance();
		GetGame().GetInputManager().AddActionListener("CVON_ShowVoiceRangeSlider", EActionTrigger.PRESSED, ShowVoiceRangeSlider);
		GetGame().GetInputManager().AddActionListener("CVON_ShowVoiceRangeSlider", EActionTrigger.UP, HideVoiceRangeSlider);
		GetGame().GetInputManager().AddActionListener("VONChannel", EActionTrigger.DOWN, ShowVON);
		GetGame().GetInputManager().AddActionListener("VONLongRange", EActionTrigger.DOWN, ShowVON);
		GetGame().GetInputManager().AddActionListener("VONMediumRange", EActionTrigger.DOWN, ShowVON);
		GetGame().GetInputManager().AddActionListener("VONRotateActive", EActionTrigger.DOWN, ShowVONActive);
		GetGame().GetInputManager().AddActionListener("VONRotateActive", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().AddActionListener("VONChannelUp", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().AddActionListener("VONChannelDown", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().AddActionListener("VONRadioEarRight", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().AddActionListener("VONRadioEarLeft", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().AddActionListener("VONRadioEarBoth", EActionTrigger.UP, HideVON);
	}
	
	//------------------------------------------------------------------------------------------------
	override event void OnStopDraw(IEntity owner)
	{
		super.OnStopDraw(owner);
		if (!CVON_VONGameModeComponent.GetInstance())
			return;
		GetGame().GetInputManager().RemoveActionListener("CVON_ShowVoiceRangeSlider", EActionTrigger.PRESSED, ShowVoiceRangeSlider);
		GetGame().GetInputManager().RemoveActionListener("CVON_ShowVoiceRangeSlider", EActionTrigger.UP, HideVoiceRangeSlider);
		GetGame().GetInputManager().RemoveActionListener("VONChannel", EActionTrigger.DOWN, ShowVON);
		GetGame().GetInputManager().RemoveActionListener("VONChannel", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().RemoveActionListener("VONLongRange", EActionTrigger.DOWN, ShowVON);
		GetGame().GetInputManager().RemoveActionListener("VONLongRange", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().RemoveActionListener("VONMediumRange", EActionTrigger.DOWN, ShowVON);
		GetGame().GetInputManager().RemoveActionListener("VONMediumRange", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().RemoveActionListener("VONRotateActive", EActionTrigger.DOWN, ShowVONActive);
		GetGame().GetInputManager().RemoveActionListener("VONRotateActive", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().RemoveActionListener("VONChannelUp", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().RemoveActionListener("VONChannelDown", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().RemoveActionListener("VONRadioEarRight", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().RemoveActionListener("VONRadioEarLeft", EActionTrigger.UP, HideVON);
		GetGame().GetInputManager().RemoveActionListener("VONRadioEarBoth", EActionTrigger.UP, HideVON);
	}
	
	//------------------------------------------------------------------------------------------------

	// UI functions

	//------------------------------------------------------------------------------------------------
	
	void ShowWarning()
	{
		m_wRoot.FindAnyWidget("VONWarning").SetOpacity(1);
	}
	
	void HideWarning()
	{
		AnimateWidget.Opacity(m_wRoot.FindAnyWidget("VONWarning"), 0, 1/0.3);
	}
	
	void ShowDirect()
	{
		#ifdef WORKBENCH
		#else
		if (SCR_PlayerController.Cast(GetGame().GetPlayerController()).GetTeamspeakClientId() == 0)
			return;
		#endif
		if (SCR_CharacterControllerComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterControllerComponent)).GetLifeState() != ECharacterLifeState.ALIVE)
			return;
		if (m_bIsToggled)
			return;
		
	 	ShowMic();
	}
	
	void ShowDirectToggle()
	{
		#ifdef WORKBENCH
		#else
		if (SCR_PlayerController.Cast(GetGame().GetPlayerController()).GetTeamspeakClientId() == 0)
			return;
		#endif
		if (SCR_CharacterControllerComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterControllerComponent)).GetLifeState() != ECharacterLifeState.ALIVE)
			return;
		GetGame().GetCallqueue().CallLater(DirectToggleDelay, 200, false);
	}
	
	void DirectToggleDelay()
	{
		m_bIsToggled = !m_bIsToggled;
		if (m_bIsToggled)
		{
			ShowMic();
			AnimateWidget.StopAnimation(m_wRoot.FindAnyWidget("Mic"), WidgetAnimationOpacity);
		}
		else
			HideDirect();
	}
	
	void ShowMic()
	{
		ImageWidget mic = ImageWidget.Cast(m_wRoot.FindAnyWidget("Mic"));
		
		switch (m_PlayerController.m_aVolumeValues.Find(m_VONGamemode.GetPlayerVolume(SCR_PlayerController.GetLocalPlayerId())))
		{
			case 0: {mic.LoadImageTexture(0, m_sMicVol1); break;}
			case 1: {mic.LoadImageTexture(0, m_sMicVol2); break;}
			case 2: {mic.LoadImageTexture(0, m_sMicVol3); break;}
			case 3: {mic.LoadImageTexture(0, m_sMicVol4); break;}
			case 4: {mic.LoadImageTexture(0, m_sMicVol5); break;}
		}
		
		mic.SetOpacity(1);
		mic.SetVisible(true);
	}
	
	void HideDirect()
	{
		if (m_bIsToggled)
			return;
		ImageWidget mic = ImageWidget.Cast(m_wRoot.FindAnyWidget("Mic"));
		mic.LoadImageTexture(0, m_sMicOff);
		mic.SetOpacity(1);
		AnimateWidget.Opacity(mic, 0, 1/0.3);
	}
	
	//Bifrost VON radio pop-up (BVON_RadioPopup.layout): replaces the old box at the left edge. The radial
	//menu still uses CVONEntry.layout through UpdateCVONRadioWidget.
	//------------------------------------------------------------------------------------------------
	static const ResourceName POPUP_LAYOUT = "{5A0F1A15E6D7CE95}UI/layouts/HUD/VON/BVON_RadioPopup.layout";
	protected Widget m_wPopupRoot;
	protected Widget m_wPopup;

	protected Widget GetPopup()
	{
		if (!m_wPopup && m_wRoot)
		{
			m_wPopupRoot = GetGame().GetWorkspace().CreateWidgets(POPUP_LAYOUT, m_wRoot);
			if (m_wPopupRoot)
			{
				FrameSlot.SetAnchorMin(m_wPopupRoot, 0, 0);
				FrameSlot.SetAnchorMax(m_wPopupRoot, 1, 1);
				FrameSlot.SetOffsets(m_wPopupRoot, 0, 0, 0, 0);
				m_wPopup = m_wPopupRoot.FindAnyWidget("BVON_RadioPopup");
			}
			//The old pop-up never shows again
			Widget oldEntry = m_wRoot.FindAnyWidget("VONEntry");
			if (oldEntry)
				oldEntry.SetVisible(false);
		}
		return m_wPopup;
	}

	static Color PopupTransmit()	{ return Color.FromRGBA(229, 146, 38, 255); }
	static Color PopupReceive()		{ return Color.FromRGBA(143, 199, 92, 255); }
	static Color PopupNeutral()		{ return Color.FromRGBA(170, 178, 166, 255); }

	protected void PopupText(string name, string text)
	{
		TextWidget w = TextWidget.Cast(m_wPopup.FindAnyWidget(name));
		if (w)
			w.SetText(text);
	}

	//Longest net and caller names the compact pop-up shows before cutting them short
	static const int POPUP_NET_CHARS = 12;
	static const int POPUP_NAME_CHARS = 24;

	static string PopupTrim(string text, int maxChars)
	{
		if (text.Length() <= maxChars)
			return text;
		return text.Substring(0, maxChars - 2) + "..";
	}

	protected void PopupColor(string name, Color color)
	{
		Widget w = m_wPopup.FindAnyWidget(name);
		if (w)
			w.SetColor(color);
	}

	//channelIndex: 0-based channel of the radio shown; frequency "" = that channel's; ear -1 = the radio's main ear
	void FillPopup(CVON_RadioComponent radioComp, int channelIndex, string frequency, int ear, string speakerName, string state, Color stateColor)
	{
		Widget popup = GetPopup();
		if (!popup || !radioComp)
			return;

		string radioName = radioComp.m_sRadioName;
		if (CVON_VehicleRadioComponent.Cast(radioComp))
			radioName = "VEHICLE RADIO";
		PopupText("PopupRadio", radioName);
		PopupText("PopupState", state);
		PopupColor("PopupState", stateColor);
		PopupColor("PopupAccent", stateColor);

		if (frequency == "")
		{
			if (channelIndex >= 0 && channelIndex < radioComp.m_aChannels.Count())
				frequency = radioComp.m_aChannels.Get(channelIndex);
			else
				frequency = radioComp.m_sFrequency;
		}
		//Compact card: channel, frequency and a short net name, the ear on the right. Monitor details stay on the
		//radio itself.
		string channelLine = string.Format("CH %1  %2", CVON_RadioMenu.PadChannel(channelIndex + 1), frequency);
		string label = radioComp.GetChannelLabel(frequency);
		if (label != "")
			channelLine += "  " + PopupTrim(label, POPUP_NET_CHARS);
		PopupText("PopupChannel", channelLine);

		CVON_EStereo shownEar = radioComp.GetEar();
		if (ear >= 0)
			shownEar = ear;
		string ears = CVON_RadioMenu.StereoText(shownEar);
		ears.ToUpper();
		PopupText("PopupEars", ears);

		//The caller's name line only while a call comes in; the box shrinks without it
		bool hasSpeaker = speakerName != "";
		PopupText("PopupSpeaker", PopupTrim(speakerName, POPUP_NAME_CHARS));
		Widget speakerWidget = m_wPopup.FindAnyWidget("PopupSpeaker");
		if (speakerWidget)
			speakerWidget.SetVisible(hasSpeaker);
		float height = 48;
		if (hasSpeaker)
			height = 68;
		Widget background = m_wPopup.FindAnyWidget("PopupBg");
		if (background)
			FrameSlot.SetSizeY(background, height);
		Widget accent = m_wPopup.FindAnyWidget("PopupAccent");
		if (accent)
			FrameSlot.SetSizeY(accent, height);

		AnimateWidget.StopAllAnimations(popup);
		popup.SetOpacity(1);
		popup.SetVisible(true);
	}

	void ShowVONChange(int input)
	{
		#ifdef WORKBENCH
		#else
		if (SCR_PlayerController.Cast(GetGame().GetPlayerController()).GetTeamspeakClientId() == 0)
			return;
		#endif
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController.m_aRadios.Count() == 0)
			return;
		IEntity radio = playerController.m_aRadios.Get(0);
		if (!radio)
			return;
		CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
		if (!radioComp)
			return;

		MarkLocalPopup();
		FillPopup(radioComp, input, "", -1, "", "", PopupNeutral());
	}

	void ShowVONActive()
	{
		#ifdef WORKBENCH
		#else
		if (SCR_PlayerController.Cast(GetGame().GetPlayerController()).GetTeamspeakClientId() == 0)
			return;
		#endif
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController.m_aRadios.Count() < 2)
			return;
		IEntity radio = playerController.m_aRadios.Get(0);
		if (!radio)
			return;
		CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
		if (!radioComp)
			return;

		MarkLocalPopup();
		FillPopup(radioComp, radioComp.m_iCurrentChannel - 1, "", -1, "", "ACTIVE", PopupNeutral());
	}

	void ShowVON()
	{
		#ifdef WORKBENCH
		#else
		if (SCR_PlayerController.Cast(GetGame().GetPlayerController()).GetTeamspeakClientId() == 0)
			return;
		#endif
		if (SCR_CharacterControllerComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterControllerComponent)).GetLifeState() != ECharacterLifeState.ALIVE)
			return;
		SCR_VONController vonController = SCR_VONController.Cast(GetGame().GetPlayerController().FindComponent(SCR_VONController));
		if (!vonController)
			return;

		if (!vonController.m_CurrentVONContainer)
			return;

		if (!Replication.FindItem(vonController.m_CurrentVONContainer.m_iRadioId))
			return;

		IEntity radio = RplComponent.Cast(Replication.FindItem(vonController.m_CurrentVONContainer.m_iRadioId)).GetEntity();
		if (!radio)
			return;
		CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
		if (!radioComp)
			return;

		MarkLocalPopup();
		FillPopup(radioComp, radioComp.m_iCurrentChannel - 1, "", -1, "", "TX", PopupTransmit());
	}

	void HideVON()
	{
		Widget popup = GetPopup();
		if (!popup)
			return;
		popup.SetOpacity(1);
		AnimateWidget.Opacity(popup, 0, 1/0.3);
	}

	//Radio pop-up while a call comes in (SCR_VONController.UpdateReceivePopup)
	//------------------------------------------------------------------------------------------------
	protected bool m_bShowingReceive = false;
	//World time (ms) of the last pop-up the player opened themselves (talking, channel, ear, rotate keys):
	//an incoming call doesn't take the pop-up over for RECEIVE_YIELD_MS after that
	protected float m_fLocalPopupTime = -100000;
	static const float RECEIVE_YIELD_MS = 1500;

	protected void MarkLocalPopup()
	{
		m_bShowingReceive = false;
		if (GetGame().GetWorld())
			m_fLocalPopupTime = GetGame().GetWorld().GetWorldTime();
	}

	//channelNumber: the radio's channel the call is on (its main or monitored channel); ear: that channel's ear
	void ShowReceiving(CVON_RadioComponent radioComp, int channelNumber, string frequency, CVON_EStereo ear, string speakerName)
	{
		if (!radioComp || !m_wRoot)
			return;
		if (GetGame().GetWorld() && GetGame().GetWorld().GetWorldTime() - m_fLocalPopupTime < RECEIVE_YIELD_MS)
			return;

		FillPopup(radioComp, channelNumber - 1, frequency, ear, speakerName, "RX", PopupReceive());
		m_bShowingReceive = true;
	}

	//The call ended: fade out, unless the pop-up is showing something the player opened since
	void HideReceiving()
	{
		if (!m_bShowingReceive)
			return;
		m_bShowingReceive = false;
		HideVON();
	}

	bool IsShowingReceive()
	{
		return m_bShowingReceive;
	}

	Widget GetPopupWidget()
	{
		return GetPopup();
	}

	static void UpdateCVONRadioWidget(CVON_RadioComponent radioComp, bool showWindow, Widget root, int input, int earOverride = -1, string speakerName = "")
	{
		ImageWidget left = ImageWidget.Cast(root.FindAnyWidget("Left"));
		ImageWidget right = ImageWidget.Cast(root.FindAnyWidget("Right"));
		ImageWidget onOff = ImageWidget.Cast(root.FindAnyWidget("On/Off"));
		ImageWidget active = ImageWidget.Cast(root.FindAnyWidget("Active"));
		TextWidget channel = TextWidget.Cast(root.FindAnyWidget("Channel"));
		TextWidget frequency = TextWidget.Cast(root.FindAnyWidget("Frequency"));
		TextWidget radio = TextWidget.Cast(root.FindAnyWidget("Radio"));
		TextWidget speaker = TextWidget.Cast(root.FindAnyWidget("Speaker"));
		WindowWidget window = WindowWidget.Cast(root.FindAnyWidget("Window"));
		WindowWidget window1 = WindowWidget.Cast(root.FindAnyWidget("Window1"));
		WindowWidget window2 = WindowWidget.Cast(root.FindAnyWidget("Window2"));

		//Radio Name text
		radio.SetText(radioComp.m_sRadioName);
		//Who is talking (only while a call comes in)
		if (speaker)
		{
			speaker.SetText(speakerName);
			speaker.SetVisible(speakerName != "");
		}
		//Freq text (a channel number past the list, e.g. one just added, shows the tuned frequency)
		if (input >= 0 && input < radioComp.m_aChannels.Count())
			frequency.SetText(radioComp.m_aChannels.Get(input));
		else
			frequency.SetText(radioComp.m_sFrequency);
		//Channel text
		channel.SetText(string.Format("CH-%1", input + 1));

		CVON_EStereo ear = radioComp.GetEar();
		if (earOverride >= 0)
			ear = earOverride;

		right.LoadImageTexture(0, CVON_HUD.m_sArrowOn);
		right.SetColor(Color.FromRGBA(180, 255, 180, 255));
		left.LoadImageTexture(0, CVON_HUD.m_sArrowOn);
		left.SetColor(Color.FromRGBA(180, 255, 180, 255));
		if (ear != CVON_EStereo.BOTH)
		{
			//Right ear
			if(ear != CVON_EStereo.RIGHT)
			{
				right.LoadImageTexture(0, CVON_HUD.m_sArrowOff);
				right.SetColor(Color.White);
			} else { // Has to be Left ear then
				left.LoadImageTexture(0, CVON_HUD.m_sArrowOff);
				left.SetColor(Color.White);
			};
		} else {
			right.LoadImageTexture(0, CVON_HUD.m_sArrowOn);
			right.SetColor(Color.FromRGBA(180, 255, 180, 255));
			left.LoadImageTexture(0, CVON_HUD.m_sArrowOn);
			left.SetColor(Color.FromRGBA(180, 255, 180, 255));
		}
		
		window.SetVisible(showWindow);
		left.SetOpacity(showWindow);
		right.SetOpacity(showWindow);
		
		window1.SetVisible(showWindow);
		window2.SetVisible(showWindow);
	}
	
	protected void ShowVoiceRangeSlider()
	{
		if (!m_InputManager || !m_wVoiceRangeSlider || !m_wVoiceRangeText) 
		{
			m_InputManager = GetGame().GetInputManager();
			m_wVoiceRangeSlider = ProgressBarWidget.Cast(m_wRoot.FindWidget("VoiceRangeSlider"));
			m_wVoiceRangeText = TextWidget.Cast(m_wRoot.FindWidget("VoiceRangeText"));
		};
		
		float currentSliderOpacity = m_wVoiceRangeSlider.GetOpacity();
		float currentTextOpacity = m_wVoiceRangeText.GetOpacity();
		
		if (currentSliderOpacity < 0.185 && currentTextOpacity < 0.185) {
			m_wVoiceRangeSlider.SetOpacity(currentSliderOpacity + 0.025);
			m_wVoiceRangeText.SetOpacity(currentTextOpacity + 0.025);
		};
		
		int actionValueUp = m_InputManager.GetActionValue("CVON_VoiceRangeUp");
		int actionValueDown = m_InputManager.GetActionValue("CVON_VoiceRangeDown");
		
		if (actionValueUp != 0 || actionValueDown != 0) {
			m_PlayerController.ChangeVoiceRange(actionValueUp + actionValueDown);
		};
		
		m_wVoiceRangeSlider.SetCurrent(m_PlayerController.ReturnLocalVoiceRange() + 1);
		ImageWidget mic = ImageWidget.Cast(m_wRoot.FindAnyWidget("Mic"));
		
		// Color
		switch (m_PlayerController.ReturnLocalVoiceRange())
		{
			case 0: 
			{ 
				m_wVoiceRangeSlider.SetColor(Color.SpringGreen);
				m_wVoiceRangeText.SetText("Whisper"); 
				m_wVoiceRangeText.SetColor(Color.SpringGreen);
				mic.LoadImageTexture(0, m_sMicVol1);
				break; 
			};
			case 1:
			{
				m_wVoiceRangeSlider.SetColor(Color.Green); 
				m_wVoiceRangeText.SetText("Close Contact"); 
				m_wVoiceRangeText.SetColor(Color.Green);
				mic.LoadImageTexture(0, m_sMicVol2);
				break; 
			};
			case 2:
			{
				m_wVoiceRangeSlider.SetColor(Color.Yellow); 
				m_wVoiceRangeText.SetText("Normal"); 
				m_wVoiceRangeText.SetColor(Color.Yellow);
				mic.LoadImageTexture(0, m_sMicVol3);
				break; 
			};
			case 3:
			{
				m_wVoiceRangeSlider.SetColor(Color.Red); 
				m_wVoiceRangeText.SetText("Yelling"); 
				m_wVoiceRangeText.SetColor(Color.Red);
				mic.LoadImageTexture(0, m_sMicVol4);
				break; 
			};
			case 4:
			{
				m_wVoiceRangeSlider.SetColor(Color.DarkRed);
				m_wVoiceRangeText.SetText("May I Speak To Your Manager");  
				m_wVoiceRangeText.SetColor(Color.DarkRed);
				mic.LoadImageTexture(0, m_sMicVol5);
				break; 
			}
			default: { m_wVoiceRangeSlider.SetColor(Color.Yellow); m_wVoiceRangeText.SetColor(Color.Yellow); break; };
		};
	};
	
	//------------------------------------------------------------------------------------------------
	protected void HideVoiceRangeSlider()
	{
		GetGame().GetCallqueue().CallLater(HideBar, 0, true);
	};


	//------------------------------------------------------------------------------------------------
	protected void HideBar()
	{
		float currentSliderOpacity = m_wVoiceRangeSlider.GetOpacity();
		float currentTextOpacity = m_wVoiceRangeText.GetOpacity();
		
		if (currentSliderOpacity <= 0 && currentTextOpacity <= 0) {
			GetGame().GetCallqueue().Remove(HideBar);
			return;
		};
		
		m_wVoiceRangeSlider.SetOpacity(currentSliderOpacity - 0.025);
		m_wVoiceRangeText.SetOpacity(currentTextOpacity - 0.025);
	}
};