//What type of transmission we are trying to send so we can send the right data to the other clients.
enum CVON_EVONTransmitType
{
	NONE,
	DIRECT,
	SR,
	LR,
	LR2
}

modded class SCR_VONController
{
	static const int CVON_DB_ATTEN_VEHICLE  = -18; // speaker inside vehicle
	static const int CVON_DB_ATTEN_BUILDING = -12; // speaker inside building

	//MMMM POINTER
	SCR_PlayerController m_PlayerController;

	SCR_BaseGameMode m_BaseGamemode;

	IEntity m_Player;

	RplComponent m_PlayerRplComponent;

	CameraManager m_CameraManager;

	MenuManager m_MenuManager;

	CameraBase m_Camera;

	//MMM... stores the gamemode
	CVON_VONGameModeComponent m_VONGameModeComponent;
	PlayerManager m_PlayerManager;

	//Who we are currently broadcasting to, this is how we know who we have to send remove calls to
	ref array<int> m_aPlayerIdsBroadcastedTo = {};

	//What is our current broadcast
	ref CVON_VONContainer m_CurrentVONContainer = null;

	//Am I broadcasting
	bool m_bIsBroadcasting = false;

	//Have I already broadcasted my current von container
	bool m_bHasBroadcasted = false;

	//Both are used so we can toggle our direct voice and use a radio at the same time
	bool m_bToggleBuffer = false;
	bool m_bToggleTurnedOffByRadio = false;

	//Stores the HUD so we can deactivate it after a transmissions end
	CVON_HUD m_VONHud;

	//Used to check life state for VON
	SCR_CharacterControllerComponent m_CharacterController;

	//Used if we are warning the player their VON is not connected after initial connection
	bool m_bShowingSecondWarning = false;

	SCR_FactionManager m_FactionManager;

	bool m_bFirstConnect = false;

	//All these below are just how we assign keybinds to activate certain VON Transmissions
	//==========================================================================================================================================================================
	void ActivateCVONSR()
	{	
		if (m_bToggleBuffer)
		{
			m_bToggleBuffer = false;
			DeactivateCVON();
			m_VONHud.ShowDirectToggle();
			m_bToggleTurnedOffByRadio = true;
		}
		ActivateCVON(CVON_EVONTransmitType.SR);
	}

	//==========================================================================================================================================================================
	void ActivateCVONLR()
	{
		if (m_bToggleBuffer)
		{
			m_bToggleBuffer = false;
			DeactivateCVON();
			m_VONHud.ShowDirectToggle();
			m_bToggleTurnedOffByRadio = true;
		}
		ActivateCVON(CVON_EVONTransmitType.LR);
	}

	//==========================================================================================================================================================================
	void ActivateCVONLR2()
	{
		if (m_bToggleBuffer)
		{
			m_bToggleBuffer = false;
			DeactivateCVON();
			m_VONHud.ShowDirectToggle();
			m_bToggleTurnedOffByRadio = true;
		}
		ActivateCVON(CVON_EVONTransmitType.LR2);
	}

	//How we rotate what radio is assigned to caps-lock, aka active.
	//==========================================================================================================================================================================
	void RotateActiveRadio()
	{
		int count = m_PlayerController.m_aRadios.Count();
		if (count < 2) return;

		IEntity last = m_PlayerController.m_aRadios[count - 1];

	    for (int i = count - 1; i > 0; i--)
	    {
	        m_PlayerController.m_aRadios[i] = m_PlayerController.m_aRadios[i - 1];
	    }
	    m_PlayerController.m_aRadios[0] = last;
		m_PlayerController.m_iAmountOfTimesRadioRotated++;
		if (m_PlayerController.m_iAmountOfTimesRadioRotated > count)
			m_PlayerController.m_iAmountOfTimesRadioRotated = 1;

	}

	//! Initialize component, done once per controller
	//==========================================================================================================================================================================
	override protected void Init(IEntity owner)
	{	
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			super.Init(owner);
			return;
		}
		if (s_bIsInit || System.IsConsoleApp())	// hosted server will have multiple controllers, init just the first one // dont init on dedicated server
		{
			Deactivate(owner);
			return;
		}

		UpdateUnconsciousVONPermitted();

		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
		{
			m_InputManager.AddActionListener(ACTION_CHANNEL, EActionTrigger.DOWN, ActivateCVONSR);
			m_InputManager.AddActionListener(ACTION_CHANNEL, EActionTrigger.UP, ReleaseRadioKey);
			m_InputManager.AddActionListener(ACTION_TRANSCEIVER_CYCLE, EActionTrigger.DOWN, ActionVONTransceiverCycle);
			m_InputManager.AddActionListener("VONLongRange", EActionTrigger.DOWN, ActivateCVONLR);
			m_InputManager.AddActionListener("VONLongRange", EActionTrigger.UP, ReleaseRadioKey);
			m_InputManager.AddActionListener("VONMediumRange", EActionTrigger.DOWN, ActivateCVONLR2);
			m_InputManager.AddActionListener("VONMediumRange", EActionTrigger.UP, ReleaseRadioKey);
			m_InputManager.AddActionListener("VONRotateActive", EActionTrigger.DOWN, RotateActiveRadio);
			m_InputManager.AddActionListener("VONChannelUp", EActionTrigger.DOWN, ChannelUp);
			m_InputManager.AddActionListener("VONChannelDown", EActionTrigger.DOWN, ChannelDown);
			m_InputManager.AddActionListener("VONOpenActive", EActionTrigger.DOWN, OpenActiveRadio);
			m_InputManager.AddActionListener("VONRadioEarRight", EActionTrigger.DOWN, SetActiveEarRight);
			m_InputManager.AddActionListener("VONRadioEarLeft", EActionTrigger.DOWN, SetActiveEarLeft);
			m_InputManager.AddActionListener("VONRadioEarBoth", EActionTrigger.DOWN, SetActiveEarBoth);
			m_InputManager.AddActionListener("VONVehicleSpeaker", EActionTrigger.DOWN, ToggleVehicleSpeaker);
			m_InputManager.AddActionListener("VONVehicleKeyFill", EActionTrigger.DOWN, KeyFillVehicleRadio);
		}

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetOwner());
		if (playerController)
		{
			OnControlledEntityChanged(null, playerController.GetControlledEntity());
			playerController.m_OnControlledEntityChanged.Insert(OnControlledEntityChanged);
		}

		PauseMenuUI.m_OnPauseMenuOpened.Insert(OnPauseMenuOpened);
		PauseMenuUI.m_OnPauseMenuClosed.Insert(OnPauseMenuClosed);

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gameMode)
			gameMode.GetOnPlayerDeleted().Insert(OnPlayerDeleted);

		m_DirectSpeechEntry = new SCR_VONEntry(); // Init direct speech entry

		ConnectToHandleUpdateVONControllersSystem();

		s_bIsInit = true;

		if (m_VONMenu)
			m_VONMenu.Init(this);

		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());

		UpdateSystemState();

		GetGame().GetCallqueue().CallLater(GetHud, 500, false);


	}

	//Change ear keybind
	//==========================================================================================================================================================================
	void SetActiveEarRight()
	{
		if (m_PlayerController.m_aRadios.Count() == 0)
			return;

		CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(m_PlayerController.m_aRadios.Get(0).FindComponent(CVON_RadioComponent));
		if (!radioComp)
			return;
		radioComp.m_eStereo = CVON_EStereo.RIGHT;
		radioComp.WriteJSON(m_Player);
		m_PlayerController.UpdateSettings();
		m_VONHud.ShowVONChange(radioComp.m_iCurrentChannel - 1);
	}

	//Change ear keybind
	//==========================================================================================================================================================================
	void SetActiveEarLeft()
	{
		if (m_PlayerController.m_aRadios.Count() == 0)
			return;

		CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(m_PlayerController.m_aRadios.Get(0).FindComponent(CVON_RadioComponent));
		if (!radioComp)
			return;
		radioComp.m_eStereo = CVON_EStereo.LEFT;
		radioComp.WriteJSON(m_Player);
		m_PlayerController.UpdateSettings();
		m_VONHud.ShowVONChange(radioComp.m_iCurrentChannel - 1);
	}

	//Change ear keybind
	//==========================================================================================================================================================================
	void SetActiveEarBoth()
	{
		if (m_PlayerController.m_aRadios.Count() == 0)
			return;

		CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(m_PlayerController.m_aRadios.Get(0).FindComponent(CVON_RadioComponent));
		if (!radioComp)
			return;
		radioComp.m_eStereo = CVON_EStereo.BOTH;
		radioComp.WriteJSON(m_Player);
		m_PlayerController.UpdateSettings();
		m_VONHud.ShowVONChange(radioComp.m_iCurrentChannel - 1);
	}

	//Keybind: turn a radio speaker on or off. In a vehicle crew seat, the vehicle radio's; otherwise our own
	//manpack's, so everyone around us hears the net.
	//==========================================================================================================================================================================
	void ToggleVehicleSpeaker()
	{
		if (!m_Player || !m_PlayerController)
			return;

		CVON_RadioComponent radio = CVON_VehicleComms.GetOperatedRadio(m_Player);
		if (!radio || !radio.CanUseSpeaker())
			radio = CVON_Manpack.GetManpackRadio(m_Player);
		//No vehicle radio or manpack: our active radio, when handheld speakers are on (TFAR style)
		if ((!radio || !radio.CanUseSpeaker()) && m_PlayerController.m_aRadios.Count() > 0 && m_PlayerController.m_aRadios[0])
		{
			CVON_RadioComponent active = CVON_RadioComponent.Cast(m_PlayerController.m_aRadios[0].FindComponent(CVON_RadioComponent));
			if (active && active.GetCarrier() == m_Player)
				radio = active;
		}
		if (!radio || !radio.CanUseSpeaker())
			return;

		m_PlayerController.ToggleVehicleSpeaker(radio.GetRplId());

		string radioName = radio.m_sRadioName;
		if (CVON_VehicleRadioComponent.Cast(radio))
			radioName = "Vehicle radio";
		// IsSpeakerOn() still has the old value until the server's change replicates
		if (radio.IsSpeakerOn())
			SCR_HintManagerComponent.ShowCustomHint(radioName + " speaker off", "Bifrost VON", 2);
		else
			SCR_HintManagerComponent.ShowCustomHint(radioName + " speaker on", "Bifrost VON", 2);
	}

	//Keybind: crew load their encryption key and channels into the vehicle radio. Until someone does, it
	//keeps its key and goes on playing that traffic.
	//==========================================================================================================================================================================
	void KeyFillVehicleRadio()
	{
		if (!m_Player || !m_PlayerController)
			return;

		CVON_VehicleRadioComponent radio = CVON_VehicleComms.GetOperatedRadio(m_Player);
		if (!radio)
			return;

		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(m_Player.FindComponent(FactionAffiliationComponent));
		if (!factionComp || factionComp.GetAffiliatedFactionKey() == "")
			return;

		if (radio.SharesKey(factionComp.GetAffiliatedFactionKey()))
		{
			SCR_HintManagerComponent.ShowCustomHint("Vehicle radio is already on your encryption", "Bifrost VON", 2);
			return;
		}

		m_PlayerController.KeyFillVehicleRadio(radio.GetRplId());
		SCR_HintManagerComponent.ShowCustomHint("Vehicle radio filled with your encryption and channels", "Bifrost VON", 3);
	}

	//Keybind to open active radio
	//==========================================================================================================================================================================
	void OpenActiveRadio()
	{
		if (m_PlayerController.m_aRadios.Count() == 0)
			return;

		CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(m_PlayerController.m_aRadios.Get(0).FindComponent(CVON_RadioComponent));
		if (!radioComp)
			return;
		radioComp.OpenMenu();
	}

	//Used as input
	//==========================================================================================================================================================================
	void ChannelUp()
	{
		ChangeChannel(1);
	}

	//used as input
	//==========================================================================================================================================================================
	void ChannelDown()
	{
		ChangeChannel(-1);
	}

	//Used for changing channel keybind to rotate through channels without opening radio
	//==========================================================================================================================================================================
	void ChangeChannel(int input)
	{
		array<IEntity> radios = m_PlayerController.m_aRadios;
		if (radios.Count() == 0)
			return;
		CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radios.Get(0).FindComponent(CVON_RadioComponent));
		if (!radioComp)
			return;
		int channelCount = radioComp.m_aChannels.Count();
		if (channelCount < 2)
			return;

		if (radioComp.m_iCurrentChannel == 1 && input == -1)
		{
			return;
		}
		else if (radioComp.m_iCurrentChannel == 99 && input == 1)
		{
			radioComp.UpdateChannelClient(1);
			string freq = radioComp.m_aChannels.Get(0);
			radioComp.UpdateFrequencyClient(freq);
			m_VONHud.ShowVONChange(0);
			return;
		}
		else if (radioComp.m_iCurrentChannel + input > channelCount)
		{
			radioComp.UpdateChannelClient(1);
			string freq = radioComp.m_aChannels.Get(0);
			radioComp.UpdateFrequencyClient(freq);
			m_VONHud.ShowVONChange(0);
			return;
		}	
		else
		{
			radioComp.UpdateChannelClient(radioComp.m_iCurrentChannel + input);
		}
		#ifdef WORKBENCH
		string freq = radioComp.m_aChannels.Get(radioComp.m_iCurrentChannel - 1);
		#else
		string freq = radioComp.m_aChannels.Get(radioComp.m_iCurrentChannel - 1 + input);
		#endif
		radioComp.UpdateFrequencyClient(freq);
		#ifdef WORKBENCH
		m_VONHud.ShowVONChange(radioComp.m_iCurrentChannel - 1);
		#else
		m_VONHud.ShowVONChange(radioComp.m_iCurrentChannel - 1 + input);
		#endif
	}

	//Fetches the VON HUD, delay is necessary.
	//==========================================================================================================================================================================
	void GetHud()
	{
		ref array<BaseInfoDisplay> displays = {};
		GetGame().GetHUDManager().GetInfoDisplays(displays);
		foreach (BaseInfoDisplay display: displays)
		{
			if (!CVON_HUD.Cast(display))
				continue;

			m_VONHud = CVON_HUD.Cast(display);
		}
	}

	//No more base game VON
	//==========================================================================================================================================================================
	override protected bool ActivateVON(notnull SCR_VONEntry entry, EVONTransmitType transmitType = EVONTransmitType.NONE)
	{
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			return super.ActivateVON(entry, transmitType);
		}
		return false;
	}

	//Radio key release tail and key discipline (ideas from Lima Charlie's release notes, our own code)
	//==========================================================================================================================================================================
	//A radio transmission ends this long after the key is let go, so the end of the last word still goes out.
	//Pressing the same radio's key again inside it carries on the same transmission: no new beeps and no new
	//entry for everyone, so a stutter on the key reads as one call.
	static const float RADIO_RELEASE_TAIL = 0.3;
	//Keying a radio more than RADIO_KEY_LIMIT times in RADIO_KEY_WINDOW seconds locks the radio keys for
	//RADIO_KEY_LOCK seconds (CVON_VONGameModeComponent.m_bRadioKeySpamLock)
	static const int RADIO_KEY_LIMIT = 4;
	static const float RADIO_KEY_WINDOW = 4;
	static const float RADIO_KEY_LOCK = 2;

	bool m_bReleasePending = false;
	float m_fReleaseAt = 0;
	ref array<float> m_aRadioKeyTimes = {};
	float m_fRadioKeysLockedUntil = 0;

	//Key up on a radio key: a radio transmission ends after the tail, anything else at once
	void ReleaseRadioKey()
	{
		if (m_CurrentVONContainer && m_CurrentVONContainer.m_eVonType == CVON_EVONType.RADIO && !m_bToggleBuffer)
		{
			m_bReleasePending = true;
			m_fReleaseAt = m_fTime + RADIO_RELEASE_TAIL;
			return;
		}
		DeactivateCVON();
	}

	//The radio a radio key transmits on (the same order as ActivateCVON below)
	IEntity RadioForTransmitType(CVON_EVONTransmitType transmitType)
	{
		int index = -1;
		switch (transmitType)
		{
			case CVON_EVONTransmitType.SR: {index = 0; break;}
			case CVON_EVONTransmitType.LR: {index = 1; break;}
			case CVON_EVONTransmitType.LR2: {index = 2; break;}
		}
		if (index < 0 || !m_PlayerController || index >= m_PlayerController.m_aRadios.Count())
			return null;
		return m_PlayerController.m_aRadios.Get(index);
	}

	//The same radio's key pressed again while its transmission is still in the release tail
	bool ContinuesPendingTransmission(CVON_EVONTransmitType transmitType)
	{
		if (!m_bReleasePending || !m_CurrentVONContainer || m_CurrentVONContainer.m_eVonType != CVON_EVONType.RADIO)
			return false;
		IEntity pressedRadio = RadioForTransmitType(transmitType);
		if (!pressedRadio)
			return false;
		RplComponent pressedRpl = RplComponent.Cast(pressedRadio.FindComponent(RplComponent));
		return pressedRpl && pressedRpl.Id() == m_CurrentVONContainer.m_iRadioId;
	}

	//Counts a radio key press. False while the radio keys are locked, or when this press locks them.
	bool AllowRadioKeyPress()
	{
		if (!m_VONGameModeComponent || !m_VONGameModeComponent.m_bRadioKeySpamLock)
			return true;
		if (m_fTime < m_fRadioKeysLockedUntil)
		{
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ACTION_FAILED);
			return false;
		}
		for (int i = m_aRadioKeyTimes.Count() - 1; i >= 0; i--)
		{
			if (m_fTime - m_aRadioKeyTimes[i] > RADIO_KEY_WINDOW)
				m_aRadioKeyTimes.Remove(i);
		}
		m_aRadioKeyTimes.Insert(m_fTime);
		if (m_aRadioKeyTimes.Count() > RADIO_KEY_LIMIT)
		{
			m_aRadioKeyTimes.Clear();
			m_fRadioKeysLockedUntil = m_fTime + RADIO_KEY_LOCK;
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.ACTION_FAILED);
			SCR_HintManagerComponent.ShowCustomHint("Radio keys locked for 2 seconds: too many key presses.", "Bifrost VON", 3);
			return false;
		}
		return true;
	}

	//This builds our VON Container with all the data we need to send to other clients based on the data sent out. This starts the talking process.
	//==========================================================================================================================================================================
	void ActivateCVON(CVON_EVONTransmitType transmitType = CVON_EVONTransmitType.NONE)
	{
		#ifdef WORKBENCH
		#else
		if (m_PlayerController.GetTeamspeakClientId() == 0 && m_VONGameModeComponent.m_bTeamspeakChecks)
			return;
		#endif
		if (!m_Player)
			return;
		if (m_CharacterController.GetLifeState() != ECharacterLifeState.ALIVE)
			return;

		bool radioKey = transmitType == CVON_EVONTransmitType.SR || transmitType == CVON_EVONTransmitType.LR || transmitType == CVON_EVONTransmitType.LR2;
		//Same radio pressed again inside the release tail: one continuous transmission
		if (radioKey && ContinuesPendingTransmission(transmitType))
		{
			m_bReleasePending = false;
			return;
		}
		if (radioKey && !AllowRadioKeyPress())
		{
			if (m_CurrentVONContainer)
				DeactivateCVON();
			return;
		}

		if (m_CurrentVONContainer)
			DeactivateCVON();
		CVON_VONContainer container = new CVON_VONContainer();
		if (transmitType == CVON_EVONTransmitType.NONE)
			return;
		if (transmitType == CVON_EVONTransmitType.DIRECT)
			container.m_eVonType = CVON_EVONType.DIRECT;
		else if (transmitType == CVON_EVONTransmitType.SR || transmitType == CVON_EVONTransmitType.LR || transmitType == CVON_EVONTransmitType.LR2)
			container.m_eVonType = CVON_EVONType.RADIO;

		container.m_iVolume = m_PlayerController.ReturnLocalVoiceVolume();
		container.m_SenderRplId = RplComponent.Cast(m_Player.FindComponent(RplComponent)).Id();
		container.m_iClientId = m_PlayerController.GetTeamspeakClientId();
		container.m_iPlayerId = m_PlayerController.GetLocalPlayerId();

		if (container.m_eVonType == CVON_EVONType.RADIO)
		{
			switch (transmitType)
			{
				case CVON_EVONTransmitType.SR:
				{
					if (m_PlayerController.m_aRadios.Count() < 1)
						return;
					IEntity radio = m_PlayerController.m_aRadios.Get(0);
					if (!radio)
					{
						//Reinitialize radios cause something is seriously fucked.
						m_PlayerController.InitializeRadios(m_PlayerController.GetControlledEntity());
						return;
					}

					if (!radio)
						return;

					CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
					if (!radioComp)
						return;
					container.m_sFrequency = radioComp.m_sFrequency;
					container.m_iRadioId = RplComponent.Cast(radio.FindComponent(RplComponent)).Id();
					container.m_sFactionKey = radioComp.m_sFactionKey;
					container.m_iTimeDeviation = radioComp.m_iTimeDeviation;
					switch (radioComp.m_eStereo)
					{
						case CVON_EStereo.BOTH:  {AudioSystem.PlaySound("{E3B4231783ABA914}UI/sounds/beepstart.wav"); break;}
						case CVON_EStereo.LEFT:  {AudioSystem.PlaySound("{3B2D6B4BBEA1CE72}UI/sounds/beepstartleft.wav"); break;}
						case CVON_EStereo.RIGHT: {AudioSystem.PlaySound("{18F289DB8B5F38D1}UI/sounds/beepstartright.wav"); break;}
					}
					break;
				}
				case CVON_EVONTransmitType.LR:
				{
					if (m_PlayerController.m_aRadios.Count() < 2)
						return;
					IEntity radio = m_PlayerController.m_aRadios.Get(1);
					if (radio == null)
						return;

					CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
					if (!radioComp)
						return;
					container.m_sFrequency = radioComp.m_sFrequency;
					container.m_iRadioId = RplComponent.Cast(radio.FindComponent(RplComponent)).Id();
					container.m_sFactionKey = radioComp.m_sFactionKey;
					container.m_iTimeDeviation = radioComp.m_iTimeDeviation;
					switch (radioComp.m_eStereo)
					{
						case CVON_EStereo.BOTH:  {AudioSystem.PlaySound("{E3B4231783ABA914}UI/sounds/beepstart.wav"); break;}
						case CVON_EStereo.LEFT:  {AudioSystem.PlaySound("{3B2D6B4BBEA1CE72}UI/sounds/beepstartleft.wav"); break;}
						case CVON_EStereo.RIGHT: {AudioSystem.PlaySound("{18F289DB8B5F38D1}UI/sounds/beepstartright.wav"); break;}
					}
					break;
				}
				case CVON_EVONTransmitType.LR2:
				{
					if (m_PlayerController.m_aRadios.Count() < 3)
						return;
					IEntity radio = m_PlayerController.m_aRadios.Get(2);
					if (radio == null)
						return;

					CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
					if (!radioComp)
						return;
					container.m_sFrequency = radioComp.m_sFrequency;
					container.m_iRadioId = RplComponent.Cast(radio.FindComponent(RplComponent)).Id();
					container.m_sFactionKey = radioComp.m_sFactionKey;
					container.m_iTimeDeviation = radioComp.m_iTimeDeviation;
					switch (radioComp.m_eStereo)
					{
						case CVON_EStereo.BOTH:  {AudioSystem.PlaySound("{E3B4231783ABA914}UI/sounds/beepstart.wav"); break;}
						case CVON_EStereo.LEFT:  {AudioSystem.PlaySound("{3B2D6B4BBEA1CE72}UI/sounds/beepstartleft.wav"); break;}
						case CVON_EStereo.RIGHT: {AudioSystem.PlaySound("{18F289DB8B5F38D1}UI/sounds/beepstartright.wav"); break;}
					}
					break;
				}
			}
		}
		m_CurrentVONContainer = container;
		m_bIsBroadcasting = true;
		m_bHasBroadcasted = false;
	}

	//No more base game VON
	//==========================================================================================================================================================================
	override protected void DeactivateVON(EVONTransmitType transmitType = EVONTransmitType.NONE)
	{
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			super.DeactivateVON(transmitType);
			return;
		}
		return;
	}

	//Stops talking and removes our VON entry from all others players that we where broadcasting to.
	//==========================================================================================================================================================================
	void DeactivateCVON()
	{
		m_bReleasePending = false;
		if (m_bToggleBuffer)
			return;
		if (!m_VONGameModeComponent)
			return;
		if (!m_CurrentVONContainer)
			return;
		if (m_CurrentVONContainer.m_eVonType == CVON_EVONType.RADIO)
		{
			RplComponent rplComp = RplComponent.Cast(Replication.FindItem(m_CurrentVONContainer.m_iRadioId));
			if (rplComp)
			{
				IEntity radio = rplComp.GetEntity();
				if (radio)
				{
					CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
					if (radioComp)
					{
						switch (radioComp.m_eStereo)
						{
							case CVON_EStereo.BOTH:  {AudioSystem.PlaySound("{B826EAACD5F6B6BB}UI/sounds/beepend.wav"); break;}
							case CVON_EStereo.LEFT:  {AudioSystem.PlaySound("{ABDEAEC2D5718124}UI/sounds/beependleft.wav"); break;}
							case CVON_EStereo.RIGHT: {AudioSystem.PlaySound("{7BF09D8FB6C39FF3}UI/sounds/beependright.wav"); break;}
						}
					}
				}
			}

			if (m_VONHud)
				m_VONHud.HideVON();
		}
		else if (m_VONHud)
			m_VONHud.HideDirect();

		// Only radio transmissions were sent to the server (see EOnFixedFrame); direct speech has nothing to remove
		bool wasRadio = m_CurrentVONContainer.m_eVonType == CVON_EVONType.RADIO;
		m_bIsBroadcasting = false;
		m_bHasBroadcasted = true;
		m_CurrentVONContainer = null;

		if (wasRadio)
			m_PlayerController.BroadcastRemoveLocalVONToServer(m_PlayerController.GetPlayerId());
	}

	//==========================================================================================================================================================================
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!CVON_VONGameModeComponent.GetInstance())
			return;
		if (System.IsConsoleApp())
			return;
		SetEventMask(owner, EntityEvent.FIXEDFRAME | EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!m_VONGameModeComponent)
			m_VONGameModeComponent = CVON_VONGameModeComponent.GetInstance();

		if (!m_PlayerManager)
			m_PlayerManager = GetGame().GetPlayerManager();

		if (!m_CameraManager)
			m_CameraManager = GetGame().GetCameraManager();

		if (!m_MenuManager)
			m_MenuManager = GetGame().GetMenuManager();
	}
	//The meat, this is where we determine who we send a VONEntry too and if we've already sent one.
	//Differentiates behavior for Direct and Radio in here as well. If a player is more than 200m away and you try to use direct he will not receive that direct VONEntry.
	//
	//Update scheduling: this used to run every fixed frame for everyone within voice range (membership,
	//stereo gains, up to ~5 physics traces per sender and a full VONData rebuild). Each part now runs at
	//the rate it needs.
	//==========================================================================================================================================================================
	static const float MEMBERSHIP_INTERVAL = 0.25;			// who is within voice range
	static const float GAIN_INTERVAL = 0.05;				// stereo gains and the VONData write (~20 Hz)
	static const int   SILENT_GAIN_DIVIDER = 4;				// players who aren't talking: every 4th gain tick (~5 Hz)
	static const float OCCLUSION_TALKING_INTERVAL = 0.25;	// muffling traces for players who are talking
	static const float OCCLUSION_SILENT_INTERVAL = 1.0;		// ... and for everyone else
	static const float PLUGIN_FILES_INTERVAL = 0.25;		// VONPluginInfo.json / VONTalking.json
	static const float CAMERA_DETACHED_DISTANCE = 30;		// metres between camera and head before hearing follows the camera
	static const int   PLUGIN_INFO_MAX_AGE = 15;			// seconds; an older info file is left over, the plugin isn't running
	static const string PLUGIN_INFO_FILE = "$profile:/VONPluginInfo.json";
	static const string TALKING_FILE = "$profile:/VONTalking.json";

	float m_fWriteTeamspeakClientIdCooldown = 0;
	ref array<int> m_PlayerIdTemp = {};
	ref set<int> m_PlayerIdSet = new set<int>();
	ref array<int> m_aEntriesToRemove = {};
	float m_fHeadCacheBuffer = 0;
	// VONServerData.json only needs to be checked at ~1 s; reading it every 50 ms
	// was 20 unnecessary file reads per second just to detect a TSClientID update.
	float m_fServerDataBuffer = 0;
	// Dirty-flag tracking: last value of IsTransmitting written to VONData.
	bool m_bLastWrittenTransmitting = false;
	// Track how many entries were in m_aLocalEntries the last time we wrote to disk.
	// If the count changes (e.g. the last speaker left range), we must force a write.
	int m_iLastWrittenEntryCount = 0;

	float m_fTime = 0;
	float m_fNextMembership = 0;
	float m_fNextGain = 0;
	float m_fNextPluginFiles = 0;
	int m_iGainTick = 0;

	// What the TeamSpeak plugin supports (VONPluginInfo.json). VONData.bin is only written for a plugin
	// that reads it; everything else gets VONData.json as before.
	bool m_bPluginReadsBinary = false;
	bool m_bLastWrittenBinary = false;
	bool m_bPluginReportsTalking = false;
	// TeamSpeak client IDs currently talking (VONTalking.json). Without it everyone counts as talking.
	ref set<int> m_TalkingClientIds = new set<int>();
	// A running plugin wrote VONPluginInfo.json, and the player's TeamSpeak state it reported: 0 or 1,
	// or -1 when the plugin doesn't say (older plugins)
	bool m_bPluginRunning = false;
	int m_iPluginConnected = -1;
	int m_iPluginInChannel = -1;
	int m_iPluginMicMuted = -1;
	int m_iPluginSoundMuted = -1;

	// Join status notice (an idea from Lima Charlie): one hint a few seconds after you first spawn on the
	// server, saying whether the plugin, TeamSpeak, the VON channel, your mic and your sound are ready
	static const float JOIN_NOTICE_DELAY = 5;		// seconds after spawning before the notice
	static const float JOIN_NOTICE_WAIT = 15;		// how long a problem may take to clear (the plugin moving you) before it's reported
	bool m_bJoinNoticeShown = false;
	float m_fJoinNoticeStart = -1;

	// Liveness signal for the TeamSpeak plugin. If the game crashes the destructor never runs, so the
	// plugin can't be told we left; instead it watches this file and, once it stops updating, releases
	// the mic, stops processing audio and returns the player to their previous channel.
	static const string HEARTBEAT_FILE = "$profile:/VONHeartbeat.json";
	static const float HEARTBEAT_INTERVAL = 1.0;
	float m_fHeartbeatTimer = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		if (m_fWriteTeamspeakClientIdCooldown > 0)
			m_fWriteTeamspeakClientIdCooldown -= timeSlice;
		else
			m_fWriteTeamspeakClientIdCooldown = 0;
		//Just in case....
		if (!CVON_VONGameModeComponent.GetInstance())
			return;

		// Before the early returns below, so it keeps beating while dead, spectating or in the respawn menu
		m_fHeartbeatTimer -= timeSlice;
		if (m_fHeartbeatTimer <= 0)
		{
			m_fHeartbeatTimer = HEARTBEAT_INTERVAL;
			WriteHeartbeat();
		}

		m_fTime += timeSlice;

		//A released radio key's tail has run out. Before the early returns below, so a transmission can't be
		//left open when the character goes away during the tail.
		if (m_bReleasePending && m_fTime >= m_fReleaseAt)
			DeactivateCVON();

		if (!m_PlayerController)
			m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());

		if (!m_BaseGamemode)
			m_BaseGamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());

		m_Player = m_PlayerController.GetControlledEntity();

		if (!m_CharacterController)
			if (m_Player)
				m_CharacterController = SCR_CharacterControllerComponent.Cast(m_Player.FindComponent(SCR_CharacterControllerComponent));

		if (!m_PlayerRplComponent)
			if (m_Player)
				m_PlayerRplComponent = RplComponent.Cast(m_Player.FindComponent(RplComponent));

		if (!m_PlayerRplComponent || !m_CharacterController || !m_Player)
			return;

		RefreshListenedRadios(timeSlice);

		m_Camera = m_CameraManager.CurrentCamera();
		if (!m_Camera)
			return;

		if (!m_bFirstConnect)
		{
			ReadPluginFiles();
			WriteJSON(true, true);
			m_bFirstConnect = true;
		}

		if (m_fTime >= m_fNextMembership)
		{
			m_fNextMembership = m_fTime + MEMBERSHIP_INTERVAL;
			UpdateMembership();
		}

		if (m_fHeadCacheBuffer >= 0.2)
		{
			UpdateHeadCache();
			m_fHeadCacheBuffer = 0;
		}
		else
			m_fHeadCacheBuffer += timeSlice;

		if (m_fTime >= m_fNextPluginFiles)
		{
			m_fNextPluginFiles = m_fTime + PLUGIN_FILES_INTERVAL;
			ReadPluginFiles();
			UpdateJoinNotice();
		}

		if (m_fTime >= m_fNextReceivePopup)
		{
			m_fNextReceivePopup = m_fTime + RECEIVE_POPUP_INTERVAL;
			UpdateReceivePopup();
		}

		//Handles broadcasting to other players
		if (m_bIsBroadcasting)
		{
			if (m_CharacterController.GetLifeState() != ECharacterLifeState.ALIVE)
			{
				if (m_bToggleBuffer)
				{
					m_bToggleBuffer = false;
					DeactivateCVON();
					m_VONHud.DirectToggleDelay();
				}
				else
					DeactivateCVON();
				return;
			}
		}

		// Only radio goes through the server. Direct speech is heard by proximity alone, so sending it was a
		// wasted round trip, and its release made the server send a remove to every player.
		if (!m_bHasBroadcasted && m_bIsBroadcasting)
		{
			if (m_CurrentVONContainer && m_CurrentVONContainer.m_eVonType == CVON_EVONType.RADIO)
				m_PlayerController.BroadcastLocalVONToServer(m_CurrentVONContainer, m_PlayerController.GetPlayerId(), m_CurrentVONContainer.m_iRadioId);
			m_bHasBroadcasted = true;
		}

		m_fServerDataBuffer += timeSlice;
		bool checkServerData = (m_fServerDataBuffer >= 1.0);
		if (checkServerData)
			m_fServerDataBuffer = 0;

		// VONData is written at ~20 Hz, but straight away when push-to-talk changes
		bool gainTick = m_fTime >= m_fNextGain;
		if (gainTick)
		{
			m_fNextGain = m_fTime + GAIN_INTERVAL;
			m_iGainTick++;
		}
		if (gainTick || checkServerData || m_bIsBroadcasting != m_bLastWrittenTransmitting)
			WriteJSON(checkServerData);
	}


	//Radio listening: rewrite RadioData.json when the target's radios change (retuned, picked up, dropped,
	//a vehicle radio) - checked once a second while listening
	//==========================================================================================================================================================================
	protected float m_fListenRadioTimer;
	protected string m_sListenRadioState;

	protected void RefreshListenedRadios(float timeSlice)
	{
		int target = m_PlayerController.GetRadioListenTarget();
		if (target == 0)
		{
			m_sListenRadioState = "";
			return;
		}

		m_fListenRadioTimer -= timeSlice;
		if (m_fListenRadioTimer > 0)
			return;

		m_fListenRadioTimer = 1;
		string state = target.ToString();
		array<RplId> radios = m_VONGameModeComponent.GetRadios(m_PlayerManager.GetPlayerControlledEntity(target));
		if (radios)
		{
			foreach (RplId radio: radios)
			{
				RplComponent radioRpl = RplComponent.Cast(Replication.FindItem(radio));
				if (!radioRpl || !radioRpl.GetEntity())
					continue;

				CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radioRpl.GetEntity().FindComponent(CVON_RadioComponent));
				if (radioComp)
					state += string.Format("|%1:%2:%3:%4:%5", radio, radioComp.m_sFrequency, radioComp.m_iTimeDeviation, radioComp.m_sFactionKey, radioComp.m_bPower);
			}
		}

		if (state == m_sListenRadioState)
			return;

		m_sListenRadioState = state;
		CVON_RadioComponent.WriteRadioData(m_Player);
	}

	//Where our radios are, for signal strength: our character - or, listening to another player's radios, theirs
	//==========================================================================================================================================================================
	vector GetRadioReceiverOrigin()
	{
		int target = m_PlayerController.GetRadioListenTarget();
		if (target != 0)
		{
			IEntity targetEntity = m_PlayerManager.GetPlayerControlledEntity(target);
			if (targetEntity)
				return targetEntity.GetOrigin();
		}

		return m_Player.GetOrigin();
	}

	//Who is within voice range. Radio entries arrive by RPC and are left alone here.
	//==========================================================================================================================================================================
	void UpdateMembership()
	{
		m_PlayerIdTemp.Clear();
		m_PlayerManager.GetPlayers(m_PlayerIdTemp);
		m_PlayerIdSet.Clear();
		foreach (int id: m_PlayerIdTemp)
			m_PlayerIdSet.Insert(id);

		// Vehicle comms: the intercom we're on, and the vehicle radio in our radio list
		m_IntercomVehicle = CVON_VehicleComms.GetIntercomVehicle(m_Player);
		RefreshVehicleComms();

		//When a player disconnects, they are no longer in the players array, so it just leaves an empty container.
		//This removes that container as when they reconnect they will no longer be heard.
		//Removals are applied after the loop: removing from a map while iterating it skips entries.
		m_aEntriesToRemove.Clear();
		foreach (int playerId, CVON_VONContainer container: m_PlayerController.m_aLocalEntries)
		{
			if (!m_PlayerIdSet.Contains(playerId) || (container.m_bIsSpectator && !IsSpectatorTalker(playerId)))
			{
				m_aEntriesToRemove.Insert(playerId);
				continue;
			}

			if (container.m_SoundSource)
			{
				int volume = m_VONGameModeComponent.GetPlayerVolume(playerId);
				float reach = volume + MEMBERSHIP_MARGIN;
				float maxDistance = reach * reach;
				container.m_iVolume = volume;

				float distance = vector.DistanceSq(container.m_SoundSource.GetOrigin(), GetListenerPosition());
				if (distance < maxDistance)
					container.m_fDistanceToSender = distance;
				else
					container.m_fDistanceToSender = -1;
			}
		}
		foreach (int playerId: m_aEntriesToRemove)
			m_PlayerController.m_aLocalEntries.Remove(playerId);

		foreach (int playerId: m_PlayerIdTemp)
		{
			if (!m_Player)
				continue;

			if (playerId == m_PlayerController.GetPlayerId())
				continue;

			CVON_VONContainer existing = m_PlayerController.m_aLocalEntries.Get(playerId);
			IEntity player = m_PlayerManager.GetPlayerControlledEntity(playerId);

			//Addon channels (e.g. spectators): heard at any distance, gains from ComputeSpectatorLR
			if (HearOverChannel(playerId))
			{
				if (!existing)
				{
					existing = new CVON_VONContainer();
					existing.m_eVonType = CVON_EVONType.DIRECT;
					existing.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
					existing.m_iClientId = m_PlayerController.GetPlayersTeamspeakClientId(playerId);
					existing.m_iPlayerId = playerId;
					if (player)
					{
						RplComponent channelRpl = RplComponent.Cast(player.FindComponent(RplComponent));
						if (channelRpl)
							existing.m_SenderRplId = channelRpl.Id();
					}
					m_PlayerController.m_aLocalEntries.Insert(playerId, existing);
				}
				existing.m_bIsSpectator = true;
				existing.m_bOnIntercom = false;
				continue;
			}

			if (!player)
			{
				//Radio and speaker entries come and go by RPC
				if (existing && !IsServerManaged(existing))
					m_PlayerController.m_aLocalEntries.Remove(playerId);
				continue;
			}

			ChimeraCharacter chimeraChar = ChimeraCharacter.Cast(player);
			if (!chimeraChar)
				continue;
			SCR_CharacterControllerComponent charCont = SCR_CharacterControllerComponent.Cast(chimeraChar.GetCharacterController());
			if (!charCont || charCont.IsDead() || charCont.IsUnconscious())
			{
				if (existing)
					m_PlayerController.m_aLocalEntries.Remove(playerId);
				continue;
			}

			//Crew on our intercom are heard at any distance between seats
			bool onIntercom = false;
			if (m_IntercomVehicle)
				onIntercom = CVON_VehicleComms.GetIntercomVehicle(player) == m_IntercomVehicle;

			float reach = m_VONGameModeComponent.GetPlayerVolume(playerId) + MEMBERSHIP_MARGIN;
			float maxDistance = reach * reach;
			float distance = vector.DistanceSq(player.GetOrigin(), GetListenerPosition());
			if (distance > maxDistance && !onIntercom)
			{
				if (existing)
				{
					existing.m_bOnIntercom = false;
					//Radio and speaker entries come and go by RPC
					if (!IsServerManaged(existing))
						m_PlayerController.m_aLocalEntries.Remove(playerId);
				}
				continue;
			}

			if (existing)
			{
				existing.m_bOnIntercom = onIntercom;
				existing.m_bIsSpectator = IsSpectatorTalker(playerId);
				continue;
			}

			CVON_VONContainer container = new CVON_VONContainer();
			container.m_eVonType = CVON_EVONType.DIRECT;
			container.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
			container.m_SenderRplId = RplComponent.Cast(player.FindComponent(RplComponent)).Id();
			container.m_iClientId = m_PlayerController.GetPlayersTeamspeakClientId(playerId);
			container.m_iPlayerId = playerId;
			container.m_bOnIntercom = onIntercom;
			container.m_bIsSpectator = IsSpectatorTalker(playerId);
			m_PlayerController.m_aLocalEntries.Insert(playerId, container);
		}
	}

	//Incoming calls: the radio pop-up shows which of our radios it is on and who is talking. A radio entry
	//exists from the sender pressing push-to-talk until they let go (the server adds and removes it), so
	//no plugin support is needed. Several at once: the one who started talking last is shown.
	//==========================================================================================================================================================================
	static const float RECEIVE_POPUP_INTERVAL = 0.1;
	float m_fNextReceivePopup = 0;
	ref array<int> m_aReceiveOrder = {};
	int m_iReceivePopupPlayer = 0;

	void UpdateReceivePopup()
	{
		if (!m_VONHud || !m_VONGameModeComponent || !m_VONGameModeComponent.m_bShowRadioCallerName)
			return;

		for (int i = m_aReceiveOrder.Count() - 1; i >= 0; i--)
		{
			CVON_VONContainer gone = m_PlayerController.m_aLocalEntries.Get(m_aReceiveOrder[i]);
			if (!gone || gone.m_eVonType != CVON_EVONType.RADIO)
				m_aReceiveOrder.RemoveOrdered(i);
		}
		foreach (int playerId, CVON_VONContainer container: m_PlayerController.m_aLocalEntries)
		{
			if (container && container.m_eVonType == CVON_EVONType.RADIO && !m_aReceiveOrder.Contains(playerId))
				m_aReceiveOrder.Insert(playerId);
		}

		//Our own transmission's pop-up wins; dead players hear nothing
		if (m_bIsBroadcasting || !m_CharacterController || m_CharacterController.GetLifeState() == ECharacterLifeState.DEAD)
		{
			m_iReceivePopupPlayer = 0;
			return;
		}

		for (int j = m_aReceiveOrder.Count() - 1; j >= 0; j--)
		{
			int talker = m_aReceiveOrder[j];
			CVON_VONContainer call = m_PlayerController.m_aLocalEntries.Get(talker);
			CVON_RadioComponent radio;
			int channel;
			CVON_EStereo ear;
			if (!FindReceivingRadio(call, radio, channel, ear))
				continue;

			m_VONHud.ShowReceiving(radio, channel, call.m_sFrequency, ear, m_PlayerManager.GetPlayerName(talker));
			m_iReceivePopupPlayer = talker;
			return;
		}

		if (m_iReceivePopupPlayer != 0)
		{
			m_iReceivePopupPlayer = 0;
			m_VONHud.HideReceiving();
		}
	}

	//Which of our radios hears this call, on which of its channels and in which ear (as the server and the
	//plugin decide: frequency, time deviation, encryption key)
	bool FindReceivingRadio(CVON_VONContainer call, out CVON_RadioComponent outRadio, out int outChannel, out CVON_EStereo outEar)
	{
		if (!call || m_PlayerController.GetHeadsetLoweredState())
			return false;

		bool encryption = m_BaseGamemode && m_BaseGamemode.IsEncryptionEnabled();
		foreach (IEntity radioEntity: m_PlayerController.m_aRadios)
		{
			if (!radioEntity)
				continue;
			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radioEntity.FindComponent(CVON_RadioComponent));
			if (!radioComp || !radioComp.m_bPower || !radioComp.ReceivesFrequency(call.m_sFrequency))
				continue;
			if (encryption && call.m_sFactionKey != radioComp.m_sFactionKey)
				continue;

			outRadio = radioComp;
			if (radioComp.m_sFrequency == call.m_sFrequency)
			{
				outChannel = radioComp.m_iCurrentChannel;
				outEar = radioComp.m_eStereo;
			}
			else
			{
				outChannel = radioComp.m_iMonitorChannel;
				outEar = radioComp.m_eMonitorStereo;
			}
			return true;
		}
		return false;
	}

	//Radio display (TX / RX marker and signal bars): is this radio transmitting (outTransmitting) or hearing a
	//call, and how strong the strongest incoming signal is (0..1, the same value the plugin turns into
	//static). False when the radio is idle.
	bool GetRadioLink(CVON_RadioComponent radio, out bool outTransmitting, out float outQuality)
	{
		outTransmitting = false;
		outQuality = 0;
		if (!radio || !m_PlayerController)
			return false;

		if (m_bIsBroadcasting && m_CurrentVONContainer && m_CurrentVONContainer.m_eVonType == CVON_EVONType.RADIO && m_CurrentVONContainer.m_iRadioId == radio.GetRplId())
		{
			outTransmitting = true;
			return true;
		}

		bool receiving = false;
		foreach (int talker, CVON_VONContainer call: m_PlayerController.m_aLocalEntries)
		{
			if (!call || call.m_eVonType != CVON_EVONType.RADIO)
				continue;
			CVON_RadioComponent hearing;
			int channel;
			CVON_EStereo ear;
			if (!FindReceivingRadio(call, hearing, channel, ear) || hearing != radio)
				continue;
			receiving = true;
			outQuality = Math.Max(outQuality, call.m_fConnectionQuality);
		}
		return receiving;
	}

	//Entries the server adds and removes (radio transmissions, and those heard through a vehicle speaker)
	static bool IsServerManaged(CVON_VONContainer container)
	{
		return container.m_eVonType == CVON_EVONType.RADIO || container.m_eVonType == CVON_EVONType.SPEAKER;
	}

	//Hooks for addons such as COALITION-Lobby (spectator channels, AAR). The defaults change nothing.
	//==========================================================================================================================================================================
	//Hear from the camera rather than the character's head (GetListenerPosition): Game Master, or any
	//camera that has left the body (a spectator camera, a dead player's camera). Addons can widen it.
	bool IsListeningFromCamera()
	{
		return IsListeningFromEditorCamera() || IsCameraDetachedFromBody();
	}

	//A talker whose gains come from ComputeSpectatorLR rather than their position (container.m_bIsSpectator)
	bool IsSpectatorTalker(int playerId)
	{
		return false;
	}

	//Hear this talker whatever the distance, even when their character isn't streamed in
	bool HearOverChannel(int playerId)
	{
		return false;
	}

	//Replace a talker's gains (e.g. silence in-world voices during the end credits); true when set
	bool OverrideTalkerGains(int playerId, out float left, out float right)
	{
		return false;
	}

	//Vehicle comms
	//==========================================================================================================================================================================
	// The vehicle whose intercom we're on (null on foot, or in a seat without one)
	IEntity m_IntercomVehicle;
	// Last vehicle we were told about, so the hint shows once per vehicle
	IEntity m_LastCommsVehicle;

	//Keeps the vehicle radio in our radio list in step with our seat, and says what's connected
	void RefreshVehicleComms()
	{
		CVON_VehicleRadioComponent radio = CVON_VehicleComms.GetOperatedRadio(m_Player);
		IEntity radioEntity = null;
		if (radio)
			radioEntity = radio.GetOwner();
		if (radioEntity != m_PlayerController.m_VehicleRadio)
			m_PlayerController.AttachVehicleRadio(m_Player);

		IEntity commsVehicle = m_IntercomVehicle;
		if (!commsVehicle)
			commsVehicle = radioEntity;
		if (commsVehicle == m_LastCommsVehicle)
			return;
		m_LastCommsVehicle = commsVehicle;
		if (!commsVehicle)
			return;

		string hint = "Vehicle radio connected";
		if (m_IntercomVehicle && radio)
			hint = "Vehicle intercom and radio connected";
		else if (m_IntercomVehicle)
			hint = "Vehicle intercom connected";

		//Radio on someone else's encryption: it keeps playing their traffic until a crew member fills it
		if (radio && radio.m_sFactionKey != "")
		{
			FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(m_Player.FindComponent(FactionAffiliationComponent));
			if (factionComp && factionComp.GetAffiliatedFactionKey() != "" && !radio.SharesKey(factionComp.GetAffiliatedFactionKey()))
			{
				//AttachVehicleRadio has just asked the server to fill it, if it may (crew comms style)
				if (radio.WouldAutoFill(m_Player))
					hint += ". Radio set to your encryption and channels.";
				else
					hint += ". The radio is on another encryption: fill it with yours to use your channels.";
			}
		}
		SCR_HintManagerComponent.ShowCustomHint(hint, "Bifrost VON", 4);
	}

	//Reads what the TeamSpeak plugin supports (VONPluginInfo.json) and who is talking (VONTalking.json).
	//==========================================================================================================================================================================
	void ReadPluginFiles()
	{
		bool binary = false;
		bool talking = false;
		bool running = false;
		int connected = -1;
		int inChannel = -1;
		int micMuted = -1;
		int soundMuted = -1;
		SCR_JsonLoadContext info = new SCR_JsonLoadContext();
		if (info.LoadFromFile(PLUGIN_INFO_FILE))
		{
			string features;
			int writtenAt;
			info.ReadValue("Features", features);
			info.ReadValue("Time", writtenAt);
			// The plugin rewrites this every few seconds and deletes it on shutdown; an old one is left
			// over from a crash or an uninstalled plugin
			int age = System.GetUnixTime() - writtenAt;
			if (age >= -5 && age <= PLUGIN_INFO_MAX_AGE)
			{
				running = true;
				binary = features.Contains("bin1");
				talking = features.Contains("talking1");
				// The player's TeamSpeak state (Bifrost VON plugin); an older plugin leaves these at -1
				if (!info.ReadValue("Connected", connected))
					connected = -1;
				if (!info.ReadValue("InChannel", inChannel))
					inChannel = -1;
				if (!info.ReadValue("MicMuted", micMuted))
					micMuted = -1;
				if (!info.ReadValue("SoundMuted", soundMuted))
					soundMuted = -1;
			}
		}
		m_bPluginRunning = running;
		m_iPluginConnected = connected;
		m_iPluginInChannel = inChannel;
		m_iPluginMicMuted = micMuted;
		m_iPluginSoundMuted = soundMuted;
		m_bPluginReadsBinary = binary;
		m_bPluginReportsTalking = talking;

		m_TalkingClientIds.Clear();
		if (!talking)
			return;

		SCR_JsonLoadContext talkingLoad = new SCR_JsonLoadContext();
		if (!talkingLoad.LoadFromFile(TALKING_FILE))
		{
			m_bPluginReportsTalking = false;
			return;
		}
		array<int> clientIds = {};
		talkingLoad.ReadValue("Talking", clientIds);
		foreach (int clientId: clientIds)
			m_TalkingClientIds.Insert(clientId);
	}

	//Shows the join status notice once, JOIN_NOTICE_DELAY after the first spawn; a problem gets until
	//JOIN_NOTICE_WAIT to clear before it is reported
	void UpdateJoinNotice()
	{
		if (m_bJoinNoticeShown || !m_PlayerController)
			return;
		if (m_fJoinNoticeStart < 0)
			m_fJoinNoticeStart = m_fTime;
		float waited = m_fTime - m_fJoinNoticeStart;
		if (waited < JOIN_NOTICE_DELAY)
			return;

		bool ready;
		string text = JoinNoticeFromState(ready);
		if (!ready && waited < JOIN_NOTICE_WAIT)
			return;

		m_bJoinNoticeShown = true;
		float duration = 15;
		if (ready)
			duration = 6;
		SCR_HintManagerComponent.ShowCustomHint(text, "Bifrost VON", duration);
	}

	string JoinNoticeFromState(out bool ready)
	{
		string version = m_PlayerController.m_sTeamspeakPluginVersion;
		bool detected = m_bPluginRunning || m_PlayerController.GetTeamspeakClientId() != 0;
		return JoinNoticeText(detected, version, m_VONGameModeComponent.m_sTeamspeakPluginVersion,
			m_VONGameModeComponent.IsPluginCompatible(version), m_VONGameModeComponent.IsPluginCurrent(version),
			m_VONGameModeComponent.m_sTeamSpeakChannelName != "",
			m_iPluginConnected, m_iPluginInChannel, m_iPluginMicMuted, m_iPluginSoundMuted, ready);
	}

	//The notice's text. ready = nothing to fix. The state values are 0 / 1, or -1 when the plugin doesn't say.
	static string JoinNoticeText(bool detected, string version, string wantedVersion, bool compatible, bool current, bool serverHasChannel,
		int connected, int inChannel, int micMuted, int soundMuted, out bool ready)
	{
		ready = false;
		if (!detected)
			return "Bifrost VON is running, but the TeamSpeak plugin was not detected. Install the Bifrost VON plugin " + wantedVersion + " and start TeamSpeak.";
		if (!compatible)
			return string.Format("Bifrost VON is running, but your TeamSpeak plugin %1 is too old for this server. Install the Bifrost VON plugin %2.", version, wantedVersion);

		array<string> problems = {};
		if (connected == 0)
			problems.Insert("TeamSpeak is not connected to a server.");
		else
		{
			if (serverHasChannel && inChannel == 0)
				problems.Insert("You are not in the VON channel. If TeamSpeak doesn't move you, ask staff to check your channel rights.");
			if (micMuted == 1)
				problems.Insert("Your microphone is muted in TeamSpeak.");
			if (soundMuted == 1)
				problems.Insert("Your sound is muted in TeamSpeak.");
		}
		if (!current)
			problems.Insert(string.Format("Your TeamSpeak plugin is %1. Install the Bifrost VON plugin %2 so radio ears play on the correct side.", version, wantedVersion));

		if (problems.IsEmpty())
		{
			ready = true;
			string readyText = "Bifrost VON is running. Plugin " + version + ".";
			if (inChannel == 1)
				readyText += " You are in the VON channel.";
			if (micMuted == 0 && soundMuted == 0)
				readyText += " Mic and sound are on.";
			return readyText;
		}

		string problemText = "Bifrost VON is running, but:";
		foreach (string problem: problems)
			problemText += "\n- " + problem;
		return problemText;
	}

	bool IsTalking(int clientId)
	{
		return !m_bPluginReportsTalking || m_TalkingClientIds.Contains(clientId);
	}

	//Voice levels: how loud a talker is in each ear, from distance, their voice range setting, walls and
	//direction. The values to tune by ear are grouped here.
	//==========================================================================================================================================================================
	static const float MAX_OUT_GAIN = 2.0;				// safety cap on per-ear gain (the loudest tier is +5 dB)
	static const float VOICE_REFERENCE_DISTANCE = 1.5;	// full volume up to this distance (m), or 30% of a shorter range
	static const float VOICE_ROLLOFF_DB = 36.0;			// how much quieter a voice is by the start of the edge fade...
	static const float VOICE_ROLLOFF_SHAPE = 0.8;		// ...falling steadily in dB across the range; below 1 drops a little faster early on
	static const float VOICE_EDGE_FADE = 0.15;			// the last 15% of the voice range fades to silence
	static const float OCCLUSION_DB_BUILDING = -8.0;	// talker or listener inside a building (the plugin also dulls the sound)
	static const float OCCLUSION_DB_VEHICLE = -12.0;	// ... inside a vehicle
	static const float STEREO_CROSSFEED = 0.25;			// share of each ear's signal also heard in the other (0 = fully one ear)
	static const float INTERCOM_GAIN = 1.0;				// per-ear gain of a talker on our vehicle intercom (the plugin adds the intercom sound)
	static const float SPEAKER_LEVEL_DB = 0.0;			// vehicle speaker loudness at close range, like a normal voice
	static const float MEMBERSHIP_MARGIN = 2.0;			// m; talkers this far past their range stay listed (silent), so the
														// edge fade is never cut short by the range check

	//Level of each voice range tier (SCR_PlayerController.m_aVolumeValues): tiers change how loud a voice
	//is, not only how far it carries
	static float VoiceTierDb(int range_m)
	{
		int tier = SCR_PlayerController.m_aVolumeValues.Find(range_m);
		switch (tier)
		{
			case 0: { return -9.0; }	// Whisper
			case 1: { return -4.0; }	// Close Contact
			case 3: { return 3.0; }		// Yelling
			case 4: { return 5.0; }		// May I Speak To Your Manager
		}
		return 0.0;						// Normal
	}

	//Walls and vehicles make a voice quieter as well as duller
	static float OcclusionDb(int loweredDecibels)
	{
		if (loweredDecibels == CVON_DB_ATTEN_VEHICLE)
			return OCCLUSION_DB_VEHICLE;
		if (loweredDecibels == CVON_DB_ATTEN_BUILDING)
			return OCCLUSION_DB_BUILDING;
		//Any other amount (e.g. an addon stacking two buildings at -24): in proportion to a building
		return loweredDecibels * (OCCLUSION_DB_BUILDING / CVON_DB_ATTEN_BUILDING);
	}

	//Full volume up close, then a steady drop in dB across the whole range (about -10 dB a quarter of the way
	//out, -20 dB at half, -28 dB at three quarters), so a voice gets clearly quieter all the way out instead of
	//staying loud until the edge. The last VOICE_EDGE_FADE then fades it to silence, from an already quiet level.
	static float DistanceGain(float d_m, float range_m)
	{
		if (range_m <= 0.01 || d_m >= range_m)
			return 0;

		// Enforce: 'reference' is reserved, and locals are function-scoped (no reusing a name in another block)
		float fullVolumeDist = Math.Min(VOICE_REFERENCE_DISTANCE, range_m * 0.3);
		float gain = 1.0;
		if (d_m > fullVolumeDist)
		{
			float rolloff = (d_m - fullVolumeDist) / (range_m - fullVolumeDist);
			gain = GainFromDb(-VOICE_ROLLOFF_DB * Math.Pow(rolloff, VOICE_ROLLOFF_SHAPE));
		}

		float x = d_m / range_m;
		float fadeStart = 1.0 - VOICE_EDGE_FADE;
		if (x > fadeStart)
		{
			float t = (x - fadeStart) / VOICE_EDGE_FADE;
			gain *= 1.0 - t * t * (3.0 - 2.0 * t);
		}
		return gain;
	}

	static float GainFromDb(float db)
	{
	    return Math.Pow(10.0, db / 20.0);
	}

	//Stereo gains for one talker. Position comes from the character's head (in third person the camera is
	//metres away, which put whispers out of range); direction comes from the camera, so left/right follow
	//where the player is looking.
	void ComputeStereoLR(
		IEntity orientation,			// the camera
		vector  listenerPos,			// the local character's head
		vector  sourcePos,				// the talker's head
		float   range_m,				// the talker's voice range
		float   levelDb,				// tier level + occlusion
		out float outBehindIntensity,	// 0 in front or at the side, 1 directly behind (drives the plugin's behind-head filter)
		out float outLeft,
		out float outRight,
		float   rearPanBoost   = 0.55,
		float   rearShadow     = 0.12,
		float   elevNarrow     = 0.25,
		bool    normalizePeak  = true
	)
	{
		outLeft = 0;
		outRight = 0;
		outBehindIntensity = 0;

	    vector Right = orientation.GetTransformAxis(0); // +X
	    vector Up    = orientation.GetTransformAxis(1); // +Y
	    vector Fwd   = orientation.GetTransformAxis(2); // +Z

	    vector toSrc = sourcePos - listenerPos;
	    float  dist  = toSrc.Length();
		float distGain = DistanceGain(dist, range_m);
		if (distGain <= 0)
			return;
	    if (dist < 0.0001) dist = 0.0001;
	    vector dir   = toSrc / dist;

	    // ---- Azimuth (project out elevation) ----
	    vector horiz = dir - Up * vector.Dot(dir, Up);
	    float  hlen  = horiz.Length();
	    if (hlen < 0.0001) { horiz = Fwd; hlen = 1.0; }
	    horiz /= hlen;

	    // ---- Pan & front/back cues ----
	    float pan    = Math.Clamp(vector.Dot(horiz, Right), -1.0, 1.0);
	    float front  = Math.Clamp(vector.Dot(horiz, Fwd),   -1.0, 1.0);
	    float back01 = Math.Pow(Math.Max(0.0, -front), 1.4);

	    // Boost panning behind to help "behindness"
	    float panScale = 1.0 + rearPanBoost * back01;
	    pan = Math.Clamp(pan * panScale, -1.0, 1.0);

	    // Narrow width when high/low
	    float elevAbs  = Math.AbsFloat(vector.Dot(dir, Up)); // 0..1
	    float width    = 1.0 - elevNarrow * elevAbs;
	    pan = Math.Clamp(pan * width, -1.0, 1.0);

	    // ---- Equal-power pan ----
	    float L = Math.Sqrt(0.5 * (1.0 - pan));
	    float R = Math.Sqrt(0.5 * (1.0 + pan));

	    // ---- Cross-feed: a voice to one side is still heard (quieter) in the other ear, as with a real head ----
	    float Lb = L, Rb = R;
	    L = (1.0 - STEREO_CROSSFEED) * Lb + STEREO_CROSSFEED * Rb;
	    R = (1.0 - STEREO_CROSSFEED) * Rb + STEREO_CROSSFEED * Lb;

	    // ---- Peak-normalize AFTER the cross-feed (so center ≈ 1/1 per ear) ----
	    if (normalizePeak) {
	        float peak = Math.Max(L, R);
	        if (peak > 0.0001) {
	            float s = 1.0 / peak;
	            L *= s;
	            R *= s;
	        }
	    }

	    // ---- Rear shadow (linear softening behind) ----
	    float rearAtt = 1.0 - rearShadow * back01;

	    float gain = rearAtt * distGain * GainFromDb(levelDb);
	    outLeft  = Math.Clamp(L * gain, 0.0, MAX_OUT_GAIN);
	    outRight = Math.Clamp(R * gain, 0.0, MAX_OUT_GAIN);

		float behindRaw = Math.Clamp((-front - 0.15) / 0.85, 0.0, 1.0);
		outBehindIntensity = behindRaw * behindRaw * (3.0 - 2.0 * behindRaw);
	}

	// Convert attenuation in dB → linear (treats +/−dB the same)
	static float AttenDbToLin(float dB)
	{
	    float a = Math.AbsFloat(dB);
	    return Math.Pow(10.0, -a / 20.0);
	}

	//Terrain: hills between two radios shrink the radio's range (the model Lima Charlie describes for its 2.0.5
	//terrain change; our own implementation). The ground is sampled along the straight line between the two
	//radios, held 1.5 m up like a handset or antenna. Each stretch rising above that line is one hill, as high
	//as the most it rises. A hill takes a share of the range that grows with its height (most from its first
	//metres, never more than 80%), and several hills each take their share of what the others left (90% at
	//most), so a ridge costs a short and a long range radio the same share rather than the same metres, and a
	//radio is never dead at point blank. Terrain only, not buildings or trees.
	//==========================================================================================================================================================================
	static const float TERRAIN_RECHECK_INTERVAL = 0.5;
	static const float RADIO_ANTENNA_HEIGHT = 1.5;
	static const float TERRAIN_SAMPLE_SPACING = 25;		//m between ground samples
	static const int TERRAIN_MIN_SAMPLES = 8;
	static const int TERRAIN_MAX_SAMPLES = 128;
	static const float TERRAIN_END_MARGIN = 10;			//m ignored at each end, so standing on a slope doesn't block your own radio
	static const float TERRAIN_HILL_CAP = 250;			//m; a hill counts as at most this high
	static const float TERRAIN_HILL_SCALE = 95;			//m; a hill this high takes 63% of the most one hill can take
	static const float TERRAIN_HILL_MAX_SHARE = 0.8;
	static const float TERRAIN_TOTAL_MAX_SHARE = 0.9;
	static const int TERRAIN_MAX_HILLS = 8;

	//Share of the radio's range one hill of this height takes
	static float HillRangeShare(float height)
	{
		if (height <= 0)
			return 0;
		return TERRAIN_HILL_MAX_SHARE * (1 - Math.Pow(2.718281828, -height / TERRAIN_HILL_SCALE));
	}

	//Share of the range several hills take together: each takes its share of what the ones before it left
	static float CombinedRangeLoss(array<float> hillHeights)
	{
		array<float> hills = {};
		hills.Copy(hillHeights);
		hills.Sort(true);
		float kept = 1;
		for (int i = 0; i < hills.Count() && i < TERRAIN_MAX_HILLS; i++)
			kept *= 1 - HillRangeShare(hills[i]);
		return Math.Min(1 - kept, TERRAIN_TOTAL_MAX_SHARE);
	}

	//Share of a radio's range (0..0.9) lost to the terrain between two radios. heightScale is the server's
	//terrain setting (CVON_VONGameModeComponent.m_fTerrainEffect).
	float TerrainRangeLoss(vector from, vector to, float heightScale = 1)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world || heightScale <= 0)
			return 0;

		from[1] = from[1] + RADIO_ANTENNA_HEIGHT;
		to[1] = to[1] + RADIO_ANTENNA_HEIGHT;
		float length = vector.Distance(from, to);
		if (length <= TERRAIN_END_MARGIN * 2)
			return 0;

		int samples = length / TERRAIN_SAMPLE_SPACING;
		samples = Math.ClampInt(samples, TERRAIN_MIN_SAMPLES, TERRAIN_MAX_SAMPLES);
		array<float> hills = {};
		float hill = 0;
		for (int i = 1; i < samples; i++)
		{
			float fraction = i;
			fraction = fraction / samples;
			float along = length * fraction;
			if (along < TERRAIN_END_MARGIN || along > length - TERRAIN_END_MARGIN)
				continue;
			vector point = from + (to - from) * fraction;
			float above = (world.GetSurfaceY(point[0], point[2]) - point[1]) * heightScale;
			if (above > 0)
				hill = Math.Max(hill, Math.Min(above, TERRAIN_HILL_CAP));
			else if (hill > 0)
			{
				hills.Insert(hill);
				hill = 0;
			}
		}
		if (hill > 0)
			hills.Insert(hill);
		return CombinedRangeLoss(hills);
	}

	//Voice directivity (as in TFAR): a voice is a little quieter behind the talker than in front of them.
	//0 dB in front, half at the side, -VOICE_DIRECTIVITY_DB straight behind; from the talker's facing.
	//==========================================================================================================================================================================
	static const float VOICE_DIRECTIVITY_DB = 5.0;

	static float TalkerFacingDb(IEntity talker, vector listenerPos)
	{
		if (!talker)
			return 0;

		vector facing = talker.GetTransformAxis(2);
		facing[1] = 0;
		vector toListener = listenerPos - talker.GetOrigin();
		toListener[1] = 0;
		if (facing.Length() < 0.001 || toListener.Length() < 0.001)
			return 0;

		float cosAngle = vector.Dot(facing.Normalized(), toListener.Normalized());
		return -VOICE_DIRECTIVITY_DB * (1.0 - cosAngle) * 0.5;
	}

	//Also bless ChatGPT, handles the arcade signal calulations.
	// distance: current distance from transmitter
	// maxDist: effective range
	// Returns: signal strength (0.0–1.0)
	//==========================================================================================================================================================================
	float GetSignalStrength(float d, float maxRangeMeters)
	{
	    if (d <= 0)                return 1.0;
	    if (d >= maxRangeMeters)   return 0.0;

	    float x = d / maxRangeMeters; // 0..1
	    float res;

	    if (x <= 0.2) {
	        float t = x / 0.2;                // 0..1 over [0.0, 0.2]
	        res = 1.0 + t * (0.9 - 1.0);      // 1.0 -> 0.9
	    }
	    else if (x <= 0.4) {
	        float t = (x - 0.2) / 0.2;        // 0..1 over (0.2, 0.4]
	        res = 0.9 + t * (0.8 - 0.9);      // 0.9 -> 0.8
	    }
	    else if (x <= 0.6) {
	        float t = (x - 0.4) / 0.2;        // 0..1 over (0.4, 0.6]
	        res = 0.8 + t * (0.5 - 0.8);      // 0.8 -> 0.5
	    }
	    else if (x <= 0.8) {
	        float t = (x - 0.6) / 0.2;        // 0..1 over (0.6, 0.8]
	        res = 0.5 + t * (0.3 - 0.5);      // 0.5 -> 0.3
	    }
	    else {
	        float t = (x - 0.8) / 0.2;        // 0..1 over (0.8, 1.0]
	        res = 0.3 + t * (0.0 - 0.3);      // 0.3 -> 0.0
	    }

	    // clamp to [0,1]
	    if (res < 0.0) res = 0.0;
	    if (res > 1.0) res = 1.0;
	    return res;
	}


	ref map<IEntity, vector> m_HeadCache = new map<IEntity, vector>;
	void UpdateHeadCache()
	{
		m_HeadCache.Clear();
		foreach (int playerId, CVON_VONContainer container: m_PlayerController.m_aLocalEntries)
		{
			if (!container.m_SoundSource)
				continue;

			if (container.m_fDistanceToSender == -1)
				continue;

			m_HeadCache.Insert(container.m_SoundSource, ComputeHeadHeight(container.m_SoundSource));
		}

		if (m_Player)
			m_HeadCache.Insert(m_Player, ComputeHeadHeight(m_Player));
	}

	vector GetHeadHeight(IEntity entity)
	{
		// A sender added since the last cache refresh isn't in it yet: compute rather than trace from (0,0,0)
		vector head;
		if (m_HeadCache.Find(entity, head))
			return head;
		head = ComputeHeadHeight(entity);
		m_HeadCache.Insert(entity, head);
		return head;
	}

	//Where the local player hears from: their character's head (third person puts the camera metres
	//away, which pushed whispers out of range), unless the camera has left the body for good
	//(IsListeningFromCamera: Game Master, spectating) - then voices are heard around the camera.
	vector GetListenerPosition()
	{
		if (m_Camera && IsListeningFromCamera())
			return m_Camera.GetOrigin();

		return GetHeadHeight(m_Player);
	}

	//Game Master. Limited editors such as the Lobby's Tactical Camera stay on the body, so a leader
	//looking down from one can't overhear voices around the camera.
	bool IsListeningFromEditorCamera()
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		return editorManager && editorManager.IsOpened() && !editorManager.IsLimited();
	}

	//The camera is nowhere near the character: a spectator camera (the Lobby's spectator character is
	//parked out of the way) or a dead player's camera. Third-person cameras, vehicles' included, stay well
	//inside CAMERA_DETACHED_DISTANCE. Limited editors (the Tactical Camera) never count.
	bool IsCameraDetachedFromBody()
	{
		if (!m_Camera || !m_Player)
			return false;

		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (editorManager && editorManager.IsOpened())
			return false;	// Game Master is handled above; limited editors stay on the body

		return vector.DistanceSq(m_Camera.GetOrigin(), GetHeadHeight(m_Player)) > CAMERA_DETACHED_DISTANCE * CAMERA_DETACHED_DISTANCE;
	}

	vector ComputeHeadHeight(IEntity entity)
	{
		Animation anim = entity.GetAnimation();
		if (!anim)
		{
			vector headOffset = "0 1.6 0";
			return entity.GetOrigin() + headOffset;
		}
		TNodeId headIndex = anim.GetBoneIndex("Head");
		vector matPos[4];
		anim.GetBoneMatrix(headIndex, matPos);
		return entity.CoordToParent(matPos[3]);

	}

	// One TraceParam reused for every muffling trace instead of allocating one per trace
	ref TraceParam m_TraceParam = new TraceParam();
	ref array<IEntity> m_aTraceExclude = {};
	static const int MAX_ROOF_TRACES = 4;

	protected float TraceOcclusion(vector start, vector end, IEntity exclude, array<IEntity> excludeArray)
	{
		m_TraceParam.Start = start;
		m_TraceParam.End = end;
		m_TraceParam.Exclude = exclude;
		m_TraceParam.ExcludeArray = excludeArray;
		m_TraceParam.Flags = TraceFlags.DEFAULT | TraceFlags.ANY_CONTACT;
		m_TraceParam.LayerMask = EPhysicsLayerDefs.Projectile;
		m_TraceParam.TraceEnt = null;
		return GetGame().GetWorld().TraceMove(m_TraceParam, null);
	}

	bool IsInBuildingOrVehicle(IEntity senderEntity, out IEntity building = null, out bool isVehicle = false)
	{
		building = null;
		isVehicle = false;
		m_aTraceExclude.Clear();
		m_aTraceExclude.Insert(senderEntity);
		vector start = GetHeadHeight(senderEntity);
		vector end = start;
		end[1] = end[1] + 10;
		// Look straight up for a roof, skipping anything that isn't one (gear, trees, props) by excluding it
		// and tracing again. Capped: this used to loop until nothing was hit.
		for (int i = 0; i < MAX_ROOF_TRACES; i++)
		{
			TraceOcclusion(start, end, null, m_aTraceExclude);
			IEntity hit = m_TraceParam.TraceEnt;
			if (!hit)
				return false;

			if (!hit.FindComponent(BaseLoadoutClothComponent))
			{
				if (Vehicle.Cast(hit.GetRootParent()))
				{
					building = hit;
					isVehicle = true;
					return true;
				}
				if (Building.Cast(hit))
				{
					building = hit;
					return true;
				}
			}
			m_aTraceExclude.Insert(hit);
		}
		return false;
	}

	//Line of sight for a voice: only what really blocks a voice counts (see LetsVoiceThrough)
	bool CanPlayerSeeSender(IEntity senderEntity, IEntity player)
	{
		float distance = TraceVoiceLine(GetHeadHeight(senderEntity), GetHeadHeight(player), player);
		return distance == 1;
	}

	float TraceVoiceLine(vector start, vector end, IEntity exclude)
	{
		m_TraceParam.Start = start;
		m_TraceParam.End = end;
		m_TraceParam.Exclude = exclude;
		m_TraceParam.ExcludeArray = null;
		m_TraceParam.Flags = TraceFlags.DEFAULT | TraceFlags.ANY_CONTACT;
		m_TraceParam.LayerMask = EPhysicsLayerDefs.Projectile;
		m_TraceParam.TraceEnt = null;
		return GetGame().GetWorld().TraceMove(m_TraceParam, BlocksVoice);
	}

	//Trace filter: true when the entity blocks a voice
	bool BlocksVoice(notnull IEntity e, vector start = "0 0 0", vector dir = "0 0 0")
	{
		return !LetsVoiceThrough(e);
	}

	//Free-standing props narrower than this on both sides don't muffle a voice
	static const float THIN_PROP_WIDTH = 1.0;

	//Things a voice carries past without being muffled (an idea from Lima Charlie): people and the gear on
	//them, trees, bushes and fallen branches, power poles and lines, see-through fences and barriers, and
	//small free-standing props. Parts of a building or vehicle (doors, windows) always block.
	static bool LetsVoiceThrough(IEntity e)
	{
		if (!e)
			return true;
		IEntity root = e.GetRootParent();
		if (ChimeraCharacter.Cast(root))
			return true;
		if (BaseTree.Cast(e) || TreeEntity.Cast(e) || SCR_TreeDebrisSmallEntity.Cast(e))
			return true;
		if (PowerPoleEntity.Cast(e) || PowerlineEntity.Cast(e))
			return true;
		if (root != e)
			return false;

		EntityPrefabData prefabData = e.GetPrefabData();
		if (prefabData)
		{
			string prefab = prefabData.GetPrefabName();
			if (prefab.Contains("Fence") || prefab.Contains("BarbedWire") || prefab.Contains("Crashbarrier"))
				return true;
		}

		vector mins, maxs;
		e.GetBounds(mins, maxs);
		float width = Math.Max(maxs[0] - mins[0], maxs[2] - mins[2]) * e.GetScale();
		return width > 0 && width < THIN_PROP_WIDTH;
	}

	void DetermineHearingWindow(IEntity entity, out float top, out float bottom)
	{
		vector origin = GetHeadHeight(entity);
		vector end = entity.GetOrigin();
		end[1] = end[1] + 100;
		float distanceUp = TraceOcclusion(origin, end, entity, null) * 100;

		end[1] = end[1] + -200;
		float distanceDown = TraceOcclusion(origin, end, entity, null) * 100;
		top = origin[1] + distanceUp;
		bottom = origin[1] - distanceDown;
	}

	// The listener's side of the muffling check is the same for every sender, so it is traced once and
	// reused instead of once per sender
	float m_fListenerOcclusionTime = -1;
	bool m_bListenerInBuilding;
	bool m_bListenerInVehicle;
	float m_fListenerTop;
	float m_fListenerBottom;
	void UpdateListenerOcclusion(IEntity player)
	{
		if (m_fListenerOcclusionTime >= 0 && m_fTime - m_fListenerOcclusionTime < OCCLUSION_TALKING_INTERVAL)
			return;
		m_fListenerOcclusionTime = m_fTime;

		IEntity building;
		bool inVehicle;
		m_bListenerInBuilding = IsInBuildingOrVehicle(player, building, inVehicle);
		m_bListenerInVehicle = inVehicle;
		if (m_bListenerInBuilding)
		{
			float top;
			float bottom;
			DetermineHearingWindow(player, top, bottom);
			m_fListenerTop = top;
			m_fListenerBottom = bottom;
		}
	}

	bool CheckIfInSameVehicle(IEntity senderEntity, IEntity player)
	{
		if (!player.GetRootParent())
			return false;

		if (!Vehicle.Cast(player.GetRootParent()))
			return false;

		SCR_BaseCompartmentManagerComponent compMan = Vehicle.Cast(player.GetRootParent()).m_CompartmentMan;
		array<BaseCompartmentSlot> compartments = {};
		compMan.GetCompartments(compartments);
		foreach (BaseCompartmentSlot compartment: compartments)
		{
			if (!compartment.IsOccupied())
				continue;

			if (compartment.GetOccupant() == senderEntity)
				return true;
		}
		return false;
	}

	bool ShouldMuffleAudio(IEntity senderEntity, int playerId = 0, out int loweredDecibles = 0)
	{
		IEntity player = m_PlayerController.GetLocalControlledEntity();
		if (!player)
			return false;

		if (!senderEntity)
			return false;

		//A free-flying camera isn't in any building or vehicle
		if (IsListeningFromCamera())
			return false;

		if (CanPlayerSeeSender(senderEntity, player))
			return false;

		//quick sanity check as the roof traces can sometimes be off if the players hatch is open
		if (CheckIfInSameVehicle(senderEntity, player))
			return false;

		UpdateListenerOcclusion(player);
		IEntity senderBuilding;
		bool isSenderInVehicle;
		bool isSenderInBuilding = IsInBuildingOrVehicle(senderEntity, senderBuilding, isSenderInVehicle);

		if (!isSenderInBuilding && !m_bListenerInBuilding)
			return false;

		if (m_bListenerInBuilding != isSenderInBuilding)
		{
			if (m_bListenerInVehicle || isSenderInVehicle)
				loweredDecibles = CVON_DB_ATTEN_VEHICLE;
			else
				loweredDecibles = CVON_DB_ATTEN_BUILDING;
			return true;
		}

		vector senderOrigin = GetHeadHeight(senderEntity);
		if (senderOrigin[1] > m_fListenerTop || senderOrigin[1] < m_fListenerBottom)
		{
			loweredDecibles = CVON_DB_ATTEN_BUILDING;
			return true;
		}
		return false;
	}

	//A talker's own voice: position, distance, voice range tier and muffling. False when they're out of
	//range or not streamed in (gains stay 0).
	//==========================================================================================================================================================================
	bool ComputeDirectLR(CVON_VONContainer container, int playerId, bool talking, out float left, out float right, out float behindIntensity, out int loweredDecibels)
	{
		left = 0;
		right = 0;
		behindIntensity = 0;
		loweredDecibels = 0;

		if (!container.m_SoundSource && container.m_fDistanceToSender != -1 && Replication.FindItem(container.m_SenderRplId))
			container.m_SoundSource = RplComponent.Cast(Replication.FindItem(container.m_SenderRplId)).GetEntity();

		if (!container.m_SoundSource || container.m_fDistanceToSender == -1)
			return false;

		if (m_fTime >= container.m_fNextOcclusionTime)
		{
			int lowered = 0;
			ShouldMuffleAudio(container.m_SoundSource, playerId, lowered);
			container.m_iLoweredDecibles = lowered;
			float occlusionInterval = OCCLUSION_SILENT_INTERVAL;
			if (talking)
				occlusionInterval = OCCLUSION_TALKING_INTERVAL;
			// Spread senders across ticks so their traces don't all land on one frame
			int stagger = playerId % 5; // integer first: Enforce has no float '%'
			container.m_fNextOcclusionTime = m_fTime + occlusionInterval + stagger * 0.02;
		}
		loweredDecibels = container.m_iLoweredDecibles;

		vector listenerPos = GetListenerPosition();
		float levelDb = VoiceTierDb(container.m_iVolume) + OcclusionDb(loweredDecibels) + TalkerFacingDb(container.m_SoundSource, listenerPos);
		ComputeStereoLR(m_Camera, listenerPos, GetHeadHeight(container.m_SoundSource), container.m_iVolume, levelDb, behindIntensity, left, right);
		return true;
	}

	//A transmission heard through a radio speaker (a vehicle's, or a manpack's): positioned at the speaker,
	//fading out over its range. Muffled when we're inside something else (a building, another vehicle); not
	//when we're in that vehicle or carrying that radio.
	//==========================================================================================================================================================================
	void ComputeSpeakerLR(CVON_VONContainer container, bool talking, out float left, out float right, out float behindIntensity, out int loweredDecibels)
	{
		left = 0;
		right = 0;
		behindIntensity = 0;
		loweredDecibels = 0;

		if (!container.m_SpeakerEntity)
		{
			RplComponent speakerRpl = RplComponent.Cast(Replication.FindItem(container.m_SpeakerRplId));
			if (speakerRpl)
				container.m_SpeakerEntity = speakerRpl.GetEntity();
			if (!container.m_SpeakerEntity)
				return;
		}
		CVON_RadioComponent speakerRadio = CVON_RadioComponent.Cast(container.m_SpeakerEntity.FindComponent(CVON_RadioComponent));
		if (!speakerRadio)
			return;
		IEntity speakerSource = speakerRadio.GetSpeakerEntity();
		if (!speakerSource)
			return;

		//Its own muffling state: the sender's direct voice may be worked out for the same entry
		if (m_fTime >= container.m_fNextSpeakerOcclusionTime)
		{
			int lowered = 0;
			if (m_Player != speakerSource && m_Player.GetRootParent() != speakerSource)
			{
				UpdateListenerOcclusion(m_Player);
				if (m_bListenerInVehicle)
					lowered = CVON_DB_ATTEN_VEHICLE;
				else if (m_bListenerInBuilding)
					lowered = CVON_DB_ATTEN_BUILDING;
			}
			container.m_iSpeakerLoweredDecibels = lowered;
			float occlusionInterval = OCCLUSION_SILENT_INTERVAL;
			if (talking)
				occlusionInterval = OCCLUSION_TALKING_INTERVAL;
			container.m_fNextSpeakerOcclusionTime = m_fTime + occlusionInterval;
		}
		loweredDecibels = container.m_iSpeakerLoweredDecibels;

		vector speakerPos = speakerRadio.GetSpeakerPosition();
		float levelDb = SPEAKER_LEVEL_DB + OcclusionDb(loweredDecibels);
		ComputeStereoLR(m_Camera, GetListenerPosition(), speakerPos, container.m_iSpeakerRange, levelDb, behindIntensity, left, right);
	}

	void ComputeSpectatorLR(int playerId, out float outLeft, out float outRight, out int silencedDecibels = 0)
	{
		//Fill in your spectator logic here
	}

	bool IsSameLanguage(int localPlayerId, int transmissionPlayerId)
	{
		bool sameLanguage = true;
		//During slotting (PREGAME) players/spectators have no faction assigned, which would otherwise
		//be misread as differing factions and babble everyone. Never babble before the round starts.
		if (m_BaseGamemode && m_BaseGamemode.GetState() == SCR_EGameModeState.PREGAME)
			return sameLanguage;

		if (m_FactionManager)
			{
				Faction localFaction = m_FactionManager.GetPlayerFaction(localPlayerId);
				Faction transmissionFaction = m_FactionManager.GetPlayerFaction(transmissionPlayerId);
				if (localFaction != transmissionFaction && m_BaseGamemode.IsBabbelEnabled())
					sameLanguage = false;
			}

		return sameLanguage;
	}


	//VONServerData.JSON
	//InGame, this is how teamspeak knows you are in game and need to be moved to the VON Channel.
	//This JSON handles everything we get from the server and teamspeak, so our TSCLientID we use to send to other clients so they know its us talking in TS.
	//VONChannelName and password so we know which channel to connect to and how to join if it has a password.
	//VONServerData
	//This iterates over all the entries in m_aLocalActiveVONEntries in our player controlller
	//ISTransmitting is how we communicate to TS that we need to open our mic in teamspeak.
	//VONType is how teamspeak knows what to do with our VONEntry, if its direct it just uses the data we give it to simualte 3D sound.
	//If its radio, it checks to see if we are on the frequency, if so applies the filters. If not then if distance is not -1 it plays it as a direct entry.
	//Frequency, this is how Teamspeak checks if a radio entry is heard on radio by you by comparing it to the data in RadioData.json.
	//Left gain and Right gain is how teamspeak determines how much each ear hears during a direct broadcast.
	//Volume is in meters your voice should travel to be heard at a decent volume, so 40 at 40m you can still hear the voice really well.
	//Distance, teamspeak uses this and volume to build a sound fall off curve and adjust the voice coming in.
	//ConnectionQuality, this handles how well your signal is to another radio, the lower the value the more distorted and quiet your voice is, as well as increasing the noise.
	//FactionKey, this always gets set but it just determines if encryption is enabled that we can hear the radio broadcast coming in.

	//Chain from broadcast is
	//You broadcast to selected players -> They receive it in their player controller and add it locally -> 
	//This WriteJson() method is being constantly called it will then write the data to the json for teamspeak to interpret ->
	//Teamspeak reads the JSON, parses the data and compares it using the clientId to see if they should hear this baffoon ->
	//The entry is removed from the local array and is no longer written to the JSON, there for teamspeak just mutes that client as he has no data in the JSON.
	//==========================================================================================================================================================================
	// checkServerData: when false, skip the VONServerData.json read entirely.
	// Called every 50ms but checkServerData is only true once per second to avoid
	// opening and parsing a file 20 times per second for data that rarely changes.
	void WriteJSON(bool checkServerData = true, bool firstConnect = false)
	{
		if (!GetGame().GetPlayerController())
			return;
		if (checkServerData)
		{
			SCR_JsonLoadContext VONLoad = new SCR_JsonLoadContext();
			if (!VONLoad.LoadFromFile("$profile:/VONServerData.json"))
			{
				SCR_JsonSaveContext VONServerData = new SCR_JsonSaveContext();
				VONServerData.StartObject("ServerData");
				VONServerData.SetMaxDecimalPlaces(1);
				VONServerData.WriteValue("InGame", true);
				VONServerData.WriteValue("InGameName", m_PlayerManager.GetPlayerName(m_PlayerController.GetPlayerId()));
				VONServerData.WriteValue("TSClientID", m_PlayerController.GetTeamspeakClientId());
				VONServerData.WriteValue("TSPluginVersion", m_PlayerController.m_sTeamspeakPluginVersion);
				VONServerData.WriteValue("VONChannelName", m_VONGameModeComponent.m_sTeamSpeakChannelName);
				VONServerData.WriteValue("VONChannelPassword", m_VONGameModeComponent.m_sTeamSpeakChannelPassword);
				VONServerData.WriteValue("TSServerIp", m_VONGameModeComponent.m_sTeamSpeakServerIP);
				VONServerData.WriteValue("TSServerPassword", m_VONGameModeComponent.m_sTeamSpeakServerPassword);
				VONServerData.EndObject();
				VONServerData.SaveToFile("$profile:/VONServerData.json");
			}
			else
			{
				string ChannelName;
				string ChannelPassword;
				int TSClientId = 0;
				bool InGame;
				string gameName;
				VONLoad.StartObject("ServerData");
				VONLoad.ReadValue("InGame", InGame);
				VONLoad.ReadValue("VONChannelName", ChannelName);
				VONLoad.ReadValue("VONChannelPassword", ChannelPassword);
				VONLoad.ReadValue("TSPluginVersion", m_PlayerController.m_sTeamspeakPluginVersion);
				VONLoad.ReadValue("TSClientID", TSClientId);
				VONLoad.ReadValue("InGameName", gameName);
				if (m_PlayerController.GetTeamspeakClientId() != TSClientId && m_fWriteTeamspeakClientIdCooldown <= 0)
				{
					m_fWriteTeamspeakClientIdCooldown = 1;
					m_PlayerController.SetTeamspeakClientId(TSClientId);
				}

				VONLoad.EndObject();

				//An older plugin (e.g. the Coalition build) still works, it only misses the Bifrost fixes: the
				//join status notice (UpdateJoinNotice) tells the player to update it

				if (gameName == "" || ChannelName != m_VONGameModeComponent.m_sTeamSpeakChannelName || ChannelPassword != m_VONGameModeComponent.m_sTeamSpeakChannelPassword || !m_VONGameModeComponent.IsPluginCompatible(m_PlayerController.m_sTeamspeakPluginVersion) || InGame != true)
				{
					SCR_JsonSaveContext VONServerData = new SCR_JsonSaveContext();
					VONServerData.StartObject("ServerData");
					VONServerData.SetMaxDecimalPlaces(1);
					VONServerData.WriteValue("InGame", true);
					VONServerData.WriteValue("InGameName", m_PlayerManager.GetPlayerName(m_PlayerController.GetPlayerId()));
					VONServerData.WriteValue("TSClientID", m_PlayerController.GetTeamspeakClientId());
					VONServerData.WriteValue("TSPluginVersion", m_PlayerController.m_sTeamspeakPluginVersion);
					VONServerData.WriteValue("VONChannelName", m_VONGameModeComponent.m_sTeamSpeakChannelName);
					VONServerData.WriteValue("VONChannelPassword", m_VONGameModeComponent.m_sTeamSpeakChannelPassword);
					VONServerData.WriteValue("TSServerIp", m_VONGameModeComponent.m_sTeamSpeakServerIP);
					VONServerData.WriteValue("TSServerPassword", m_VONGameModeComponent.m_sTeamSpeakServerPassword);
					VONServerData.EndObject();
					VONServerData.SaveToFile("$profile:/VONServerData.json");
				}
			}
		}
		#ifdef ENABLE_DIAG
		#else
		//Hijack this whole process to load the initial warning menu
		if (m_VONGameModeComponent.m_bTeamspeakChecks)
		{	
			if (m_PlayerController.GetTeamspeakClientId() == 0 && !m_PlayerController.m_bHasConnectedToTeamspeakForFirstTime)
			{
				if (!m_MenuManager.GetTopMenu())
					m_MenuManager.OpenMenu(ChimeraMenuPreset.CVON_WarningMenu);
				else if (!m_MenuManager.GetTopMenu().IsInherited(CVON_WarningMenu) && !m_MenuManager.GetTopMenu().IsInherited(PauseMenuUI))
					m_MenuManager.OpenMenu(ChimeraMenuPreset.CVON_WarningMenu);
			}

		}
		#endif
		// The camera gives listening direction, the character's head the listening position
		if (!m_Camera || !m_Player)
			return;

		if (!m_FactionManager)
			m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());

		if (!m_BaseGamemode)
			m_BaseGamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());

		// Dirty flag: if nothing changed since the last write, skip the write entirely. Also dirty when the
		// entry count changes — this catches the case where the last speaker leaves range and is removed
		// from m_aLocalEntries entirely: the loop below never runs, so only this count comparison can
		// trigger the flush that removes their entry and lets TeamSpeak mute them.
		int currentEntryCount = m_PlayerController.m_aLocalEntries.Count();
		bool dirty = firstConnect ||
					 (m_bIsBroadcasting != m_bLastWrittenTransmitting) ||
					 (currentEntryCount != m_iLastWrittenEntryCount) ||
					 (m_bPluginReadsBinary != m_bLastWrittenBinary);

		// Work out each entry. Players who aren't talking are refreshed every SILENT_GAIN_DIVIDER-th tick,
		// and the muffling traces run on their own slower schedule per sender.
		const float EPS = 0.005;
		int silentTickPhase = m_iGainTick % SILENT_GAIN_DIVIDER;
		foreach (int playerId, CVON_VONContainer container: m_PlayerController.m_aLocalEntries)
		{
			int clientId = m_PlayerController.GetPlayersTeamspeakClientId(playerId);
			bool talking = IsTalking(clientId);
			bool fresh = container.m_fCachedLeft == -999;
			if (!talking && !fresh && clientId == container.m_iCachedClientId && silentTickPhase != 0)
				continue; // keep the values from the last refresh

			float left = 0;
			float right = 0;
			float behindIntensity = 0;
			int loweredDecibels = 0;
			string frequency = container.m_sFrequency;
			int outType = container.m_eVonType;
			if (!m_CharacterController || m_CharacterController.GetLifeState() == ECharacterLifeState.DEAD)
			{
				//Cuts off all incoming audio, cause we're dead.
				frequency = "";
			}
			else if (container.m_bIsSpectator)
			{
				ComputeSpectatorLR(playerId, left, right);
			}
			else if (OverrideTalkerGains(playerId, left, right))
			{
				//Gains set by an addon
			}
			else if (container.m_bOnIntercom)
			{
				//Same vehicle intercom: clear and centred, whatever the engine noise, seat distance or their
				//voice range. Wins over radio, as their radio calls are on the intercom too.
				outType = CVON_EVONType.INTERCOM;
				left = INTERCOM_GAIN;
				right = INTERCOM_GAIN;
			}
			else if (container.m_eVonType == CVON_EVONType.SPEAKER)
			{
				ComputeSpeakerLR(container, talking, left, right, behindIntensity, loweredDecibels);

				//The sender may also be close enough to hear directly. One voice per talker, so whichever is
				//louder here wins: next to the vehicle the speaker, next to the sender their own voice.
				float directLeft, directRight, directBehind;
				int directLowered;
				if (ComputeDirectLR(container, playerId, talking, directLeft, directRight, directBehind, directLowered) && directLeft + directRight > left + right)
				{
					outType = CVON_EVONType.DIRECT;
					left = directLeft;
					right = directRight;
					behindIntensity = directBehind;
					loweredDecibels = directLowered;
				}
			}
			else
			{
				ComputeDirectLR(container, playerId, talking, left, right, behindIntensity, loweredDecibels);
			}

			if (container.m_eVonType == CVON_EVONType.RADIO)
			{
				vector senderPos = container.m_vSenderLocation;
				if (container.m_SoundSource)
					senderPos = container.m_SoundSource.GetOrigin();
				vector receiverPos = GetRadioReceiverOrigin();

				//A Game Master in the editor hears every call clearly (m_bGameMasterUnlimitedRadio)
				if (m_VONGameModeComponent.m_bGameMasterUnlimitedRadio && IsListeningFromEditorCamera())
					container.m_fConnectionQuality = 1;
				else
				{
					//Terrain between us shrinks the sender's range
					float terrainLoss = 0;
					if (m_VONGameModeComponent.m_fTerrainEffect > 0)
					{
						if (m_fTime >= container.m_fNextTerrainTime)
						{
							container.m_fTerrainObstruction = TerrainRangeLoss(senderPos, receiverPos, m_VONGameModeComponent.m_fTerrainEffect);
							container.m_fNextTerrainTime = m_fTime + TERRAIN_RECHECK_INTERVAL;
						}
						terrainLoss = container.m_fTerrainObstruction;
					}
					container.m_fConnectionQuality = GetSignalStrength(vector.Distance(receiverPos, senderPos), container.m_iMaxDistance * (1 - terrainLoss));
				}
			}
			else if (container.m_eVonType == CVON_EVONType.SPEAKER && container.m_SpeakerEntity)
			{
				//The vehicle's radio is the receiver: its distance from the sender sets the signal
				container.m_fConnectionQuality = GetSignalStrength(vector.Distance(container.m_SpeakerEntity.GetOrigin(), container.m_vSenderLocation), container.m_iMaxDistance);
			}

			bool sameLanguage = IsSameLanguage(m_PlayerController.GetPlayerId(), container.m_iPlayerId);

			// Check if this entry differs enough from what was last written (epsilon ~0.5%).
			if (!dirty)
			{
				if (Math.AbsFloat(left  - container.m_fCachedLeft)   > EPS ||
					Math.AbsFloat(right - container.m_fCachedRight)  > EPS ||
					Math.AbsFloat(behindIntensity - container.m_fCachedBehind) > EPS ||
					Math.AbsFloat(container.m_fConnectionQuality - container.m_fCachedConnQ) > EPS ||
					loweredDecibels  != container.m_iCachedDecibels ||
					sameLanguage     != container.m_bCachedSameLang ||
					frequency        != container.m_sCachedFreq ||
					clientId         != container.m_iCachedClientId ||
					outType          != container.m_iCachedType)
					dirty = true;
			}

			container.m_fCachedLeft     = left;
			container.m_fCachedRight    = right;
			container.m_fCachedBehind   = behindIntensity;
			container.m_iCachedDecibels = loweredDecibels;
			container.m_fCachedConnQ    = container.m_fConnectionQuality;
			container.m_bCachedSameLang = sameLanguage;
			container.m_sCachedFreq     = frequency;
			container.m_iCachedClientId = clientId;
			container.m_iCachedType     = outType;
		}

		if (!dirty)
			return;

		if (m_bPluginReadsBinary)
			WriteVONDataBinary();
		else
			WriteVONDataJson();
		m_bLastWrittenTransmitting = m_bIsBroadcasting;
		m_iLastWrittenEntryCount = currentEntryCount;
		m_bLastWrittenBinary = m_bPluginReadsBinary;
	}

	//VONData.json: what every plugin version reads
	//==========================================================================================================================================================================
	void WriteVONDataJson()
	{
		SCR_JsonSaveContext VONSave = new SCR_JsonSaveContext();
		VONSave.WriteValue("IsTransmitting", m_bIsBroadcasting);
		foreach (int playerId, CVON_VONContainer container: m_PlayerController.m_aLocalEntries)
		{
			VONSave.StartObject(container.m_iCachedClientId.ToString());
			VONSave.SetMaxDecimalPlaces(3);
			VONSave.WriteValue("VONType", container.m_iCachedType);
			VONSave.WriteValue("Frequency", container.m_sCachedFreq);
			VONSave.WriteValue("LeftGain", container.m_fCachedLeft);
			VONSave.WriteValue("RightGain", container.m_fCachedRight);
			VONSave.WriteValue("MuffledDecibels", container.m_iCachedDecibels);
			VONSave.WriteValue("ConnectionQuality", container.m_fCachedConnQ);
			VONSave.WriteValue("FactionKey", container.m_sFactionKey);
			VONSave.WriteValue("BehindIntensity", container.m_fCachedBehind);
			VONSave.WriteValue("SameLanguage", container.m_bCachedSameLang);
			VONSave.EndObject();
		}
		VONSave.SaveToFile("$profile:/VONData.json");
	}

	//VONData.bin: the same data packed into fixed records, for plugins that list "bin1" in VONPluginInfo.json.
	//About a tenth of the size, no text formatting or parsing, and a sequence number at both ends lets the
	//plugin detect a half-written file. Layout (little-endian 4-byte words) is documented in the plugin's
	//load_von_bin_into; keep the two in step and bump VONBIN_VERSION on any change.
	//==========================================================================================================================================================================
	static const string VONDATA_BIN_FILE = "$profile:/VONData.bin";
	static const int VONBIN_MAGIC = 0x4E4F5643; // "CVON"
	static const int VONBIN_VERSION = 1;
	static const int VONBIN_ENTRY_WORDS = 5;
	static const int VONBIN_NO_STRING = 0xFFFF;
	static const int VONBIN_NO_TIME_DEVIATION = 0x7FFFFFFF; // the JSON format never sent one either
	int m_iBinarySeq = 0;
	ref array<int> m_aBinWords = {};
	ref array<string> m_aBinStrings = {};
	ref map<string, int> m_mBinStringIndex = new map<string, int>();

	protected int BinStringIndex(string value)
	{
		if (value.IsEmpty())
			return VONBIN_NO_STRING;
		int index;
		if (m_mBinStringIndex.Find(value, index))
			return index;
		index = m_aBinStrings.Count();
		m_aBinStrings.Insert(value);
		m_mBinStringIndex.Insert(value, index);
		return index;
	}

	protected int ToByte(float value01)
	{
		return (int)Math.Round(Math.Clamp(value01, 0, 1) * 255);
	}

	protected int ToGain(float gain)
	{
		return (int)Math.Round(Math.Clamp(gain, 0, 6.5) * 10000);
	}

	void WriteVONDataBinary()
	{
		m_aBinWords.Clear();
		m_aBinStrings.Clear();
		m_mBinStringIndex.Clear();
		m_iBinarySeq++;

		int flags = 0;
		if (m_bIsBroadcasting)
			flags = 1;

		// Header; the entry and string counts are filled in below
		m_aBinWords.Insert(VONBIN_MAGIC);
		m_aBinWords.Insert(VONBIN_VERSION | (flags << 16));
		m_aBinWords.Insert(m_iBinarySeq);
		m_aBinWords.Insert(0);
		m_aBinWords.Insert(0);

		int count = 0;
		foreach (int playerId, CVON_VONContainer container: m_PlayerController.m_aLocalEntries)
		{
			int entryFlags = 0;
			if (container.m_bCachedSameLang)
				entryFlags = 1;
			int muffled = container.m_iCachedDecibels;

			m_aBinWords.Insert((container.m_iCachedClientId & 0xFFFF) | ((container.m_iCachedType & 0xFF) << 16) | (entryFlags << 24));
			m_aBinWords.Insert((ToGain(container.m_fCachedLeft) & 0xFFFF) | (ToGain(container.m_fCachedRight) << 16));
			m_aBinWords.Insert((BinStringIndex(container.m_sCachedFreq) & 0xFFFF) | (BinStringIndex(container.m_sFactionKey) << 16));
			m_aBinWords.Insert(VONBIN_NO_TIME_DEVIATION);
			m_aBinWords.Insert((muffled & 0xFF) | (ToByte(container.m_fCachedConnQ) << 8) | (ToByte(container.m_fCachedBehind) << 16));
			count++;
		}
		m_aBinWords.Set(3, (count & 0xFFFF) | (VONBIN_ENTRY_WORDS << 16));
		m_aBinWords.Set(4, m_aBinStrings.Count() & 0xFFFF);

		FileHandle file = FileIO.OpenFile(VONDATA_BIN_FILE, FileMode.WRITE);
		if (!file)
			return;
		file.WriteArray(m_aBinWords, 4);
		foreach (string value: m_aBinStrings)
		{
			int length = value.Length();
			file.Write(length, 4);
			file.Write(value, length);
		}
		file.Write(m_iBinarySeq, 4); // trailer: must match the header for the plugin to accept the file
		file.Close();
	}

	//See HEARTBEAT_FILE. The plugin only looks at when the file was last written; the timestamp inside is for debugging.
	//==========================================================================================================================================================================
	void WriteHeartbeat()
	{
		SCR_JsonSaveContext heartbeat = new SCR_JsonSaveContext();
		heartbeat.WriteValue("Heartbeat", System.GetUnixTime());
		heartbeat.SaveToFile(HEARTBEAT_FILE);
	}

	//Resets these values so we can leave the channel on teamspeak
	//It checks if the bool InGame is true, and if so moves you to the voip channel
	//==========================================================================================================================================================================
	void ~SCR_VONController()
	{
		// Leaving mid-transmission must not leave the mic open: older plugins only look at IsTransmitting
		if (!System.IsConsoleApp())
		{
			SCR_JsonSaveContext VONSave = new SCR_JsonSaveContext();
			VONSave.WriteValue("IsTransmitting", false);
			VONSave.SaveToFile("$profile:/VONData.json");
		}

		if (m_aPlayerIdsBroadcastedTo.Count() > 0)
		{
			foreach (int playerId: m_aPlayerIdsBroadcastedTo)
			{
				m_PlayerController.BroadcastRemoveLocalVONToServer(m_PlayerController.GetPlayerId());
			}
			m_aPlayerIdsBroadcastedTo.Clear();
		}

		SCR_JsonSaveContext VONServerData = new SCR_JsonSaveContext();
		VONServerData.StartObject("ServerData");
		VONServerData.WriteValue("InGame", false);
		VONServerData.WriteValue("InGameName", "");
		VONServerData.WriteValue("TSClientID", 0);
		VONServerData.WriteValue("TSPluginVersion", 0);
		VONServerData.WriteValue("VONChannelName", "");
		VONServerData.WriteValue("VONChannelPassword", "");
		VONServerData.WriteValue("TSServerIp", "");
		VONServerData.WriteValue("TSServerPassword", "");
		VONServerData.EndObject();
		VONServerData.SaveToFile("$profile:/VONServerData.json");
	}
}