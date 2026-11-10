modded class SCR_GroupsManagerComponent
{
	//Called when spawning in. All this handles automatically assigning frequencies when you join a group.
	//==========================================================================================================================================================================
	override void TunePlayersFrequency(int playerId, IEntity player)
	{
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			super.TunePlayersFrequency(playerId, player);
			return;
		}
		GetGame().GetCallqueue().CallLater(TuneFreqDelayWithPresets, 600, false, playerId, player);
	}
	
	//A player's saved radio settings by radio type ("AN/PRC-152#0"), as SCR_PlayerController.RadioSettingKey names them
	static map<string, ref CVON_RadioSettingObject> SavedRadioSettings(SCR_PlayerController playerController)
	{
		map<string, ref CVON_RadioSettingObject> byKey = new map<string, ref CVON_RadioSettingObject>();
		foreach (CVON_RadioSettingObject saved: playerController.m_aRadioSettings)
		{
			if (saved && saved.m_sKey != "")
				byKey.Set(saved.m_sKey, saved);
		}
		return byKey;
	}

	//Is there any preset frequencies, used to determine if we use these customs freqs or vanilla.
	bool AnyPlayerFrequencies(int playerId)
	{
		if (!CVON_VONGameModeComponent.GetInstance().m_FreqConfig)
			return false;
		
		if (CVON_VONGameModeComponent.GetInstance().m_FreqConfig.m_aPresetGroupFrequencyContainers.Count() > 0)
			return true;
		
		return false;
	}
	
	//Needed as if you character has already been spawned and you make a new group this is what it calls, because YES!
	//==========================================================================================================================================================================
	override private void TuneAgentsRadio(AIAgent agentEntity)
	{
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			super.TuneAgentsRadio(agentEntity);
			return;
		}
		IEntity player = agentEntity.GetControlledEntity();
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(player);
		if (playerId == -1)
			return;
		GetGame().GetCallqueue().CallLater(TuneFreqDelayWithPresets, 600, false, playerId, player);
	}
	
	//Needed so we wait for the group to initialize
	//==========================================================================================================================================================================
	void TuneFreqDelayWithPresets(int playerId, IEntity player)
	{
		if (!player)
			return;
		
		if (!AnyPlayerFrequencies(playerId))
		{
			TuneFreqWithoutPresets(playerId, player);
			return;
		}
			
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;
		
		int SRIndex = 0;
		int LRIndex = 0;
		array<IEntity> radiosDone = {};
		if (playerController.m_aRadioSettings.Count() > 0)
		{
			map<string, ref CVON_RadioSettingObject> savedByKey = SavedRadioSettings(playerController);
			map<string, int> nameCounts = new map<string, int>();
			foreach (IEntity radio: playerController.m_aRadios)
			{
				//A radio dropped or deleted since the list was built
				if (!radio)
					continue;
				CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
				if (!radioComp)
					continue;
				//Matched by radio type, not list position: the loadout can differ after a respawn
				CVON_RadioSettingObject radioSetting;
				if (!savedByKey.Find(SCR_PlayerController.RadioSettingKey(radioComp, nameCounts), radioSetting))
					continue;

				if (radioComp.m_aChannels.Contains(radioSetting.m_sFreq))
				{
					radioComp.UpdateChannelServer(radioComp.m_aChannels.Find(radioSetting.m_sFreq) + 1);
					radioComp.UpdateFrequncyServer(radioSetting.m_sFreq, -1, false);
					radiosDone.Insert(radio);
					if (radioComp.m_eRadioType == 0)
						SRIndex++;
					else
						LRIndex++;
				}			
			}
		}
		
		SCR_Faction playerFaction = SCR_Faction.Cast(SCR_FactionManager.Cast(GetGame().GetFactionManager()).GetPlayerFaction(playerId));
		if (!playerFaction)
			return;

		array<SCR_AIGroup> groups = GetPlayableGroupsByFaction(playerFaction);
		if (!groups)
			return;
		SCR_AIGroup playersGroup = GetPlayerGroup(playerId);
		int index = groups.Find(playersGroup);
		if (index == -1)
			return;
	
		//Gonna get the groups callsign so we know which frequency object is theres
		string company;
		string platoon;
		string squad;
		string character;
		string format;
		playersGroup.GetCallsigns(company, platoon, squad, character, format);
		string playersGroupName = string.Format(format, company, platoon, squad, character);
		CVON_GroupFrequencyContainer freqContainer;
		CVON_VONGameModeComponent gamemodeComp = CVON_VONGameModeComponent.GetInstance();
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (gamemodeComp.m_FreqConfig)
		{
			foreach (CVON_GroupFrequencyContainer freqItem: gamemodeComp.m_FreqConfig.m_aPresetGroupFrequencyContainers)
			{
				foreach (string groupName: freqItem.m_aGroupNames)
				{
					if (groupName != playersGroupName && groupName != playersGroup.GetCustomNameServer())
						continue;
					
					freqContainer = freqItem;
					break;
				}
			}
		}
		array<ref CVON_GroupFrequencyContainer> factionOverrides;
		if (playerFaction.GetCallsignInfo())
			factionOverrides = playerFaction.GetCallsignInfo().m_aGroupFrequencyOverrides;
		if (!factionOverrides)
			factionOverrides = {};
		foreach (CVON_GroupFrequencyContainer container: factionOverrides)
		{
			foreach (string groupName: container.m_aGroupNames)
			{
				if (groupName != playersGroupName && groupName != playersGroup.GetCustomNameServer())
					continue;
				
				freqContainer = container;
				break;
			}
		}
		for (int i = 0; i < playerController.m_aRadios.Count(); i++)
		{
			if (radiosDone.Contains(playerController.m_aRadios.Get(i)))
				continue;
			bool m_bFrequencyFound = false;
			IEntity radioEntity = playerController.m_aRadios.Get(i);
			if (!radioEntity)
				continue;
			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radioEntity.FindComponent(CVON_RadioComponent));
			if (!radioComp)
				continue;
			if (!freqContainer)
			{
				radioComp.UpdateChannelServer(1);
				if (radioComp.m_aChannels.Count() > 0)
					radioComp.UpdateFrequncyServer(radioComp.m_aChannels.Get(0));
				else
					radioComp.UpdateFrequncyServer("55500");
				continue;
			}
			switch (radioComp.m_eRadioType)
			{
				case CVON_ERadioType.SHORT: 
				{
					if (!freqContainer.m_aSRFrequencies)
						break;
					if (freqContainer.m_aSRFrequencies.Count() < SRIndex + 1)
						break;
					string freq = freqContainer.m_aSRFrequencies.Get(SRIndex);
					if (!radioComp.m_aChannels.Contains(freq))
						break;
					SRIndex++;
					radioComp.UpdateChannelServer(radioComp.m_aChannels.Find(freq) + 1);
					radioComp.UpdateFrequncyServer(freq);
					m_bFrequencyFound = true;
					break;
				}
				case CVON_ERadioType.LONG: 
				{
					if (!freqContainer.m_aLRFrequencies)
						break;
					if (freqContainer.m_aLRFrequencies.Count() < LRIndex + 1)
						break;
					string freq = freqContainer.m_aLRFrequencies.Get(LRIndex);
					if (!radioComp.m_aChannels.Contains(freq))
						break;
					LRIndex++;
					radioComp.UpdateChannelServer(radioComp.m_aChannels.Find(freq) + 1);
					radioComp.UpdateFrequncyServer(freq);
					m_bFrequencyFound = true;
					break;
				}
			}
			if (m_bFrequencyFound)
				continue;
			
			radioComp.UpdateChannelServer(1);
			if (radioComp.m_aChannels.Count() > 0)
				radioComp.UpdateFrequncyServer(radioComp.m_aChannels.Get(0));
			else
				radioComp.UpdateFrequncyServer("55500");
		}
	}
	
	
	//Used if no frequency config is added or is not populated, just the vanilla frequencies.
	//==========================================================================================================================================================================
	void TuneFreqWithoutPresets(int playerId, IEntity player)
	{
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;
		
		//Radios that get their saved frequency back; any others (a radio picked up since) are tuned below
		array<IEntity> restored = {};
		if (playerController.m_aRadioSettings.Count() > 0)
		{
			map<string, ref CVON_RadioSettingObject> savedByKey = SavedRadioSettings(playerController);
			map<string, int> nameCounts = new map<string, int>();
			foreach (IEntity radio: playerController.m_aRadios)
			{
				if (!radio)
					continue;
				CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
				if (!radioComp)
					continue;
				CVON_RadioSettingObject radioSetting;
				if (!savedByKey.Find(SCR_PlayerController.RadioSettingKey(radioComp, nameCounts), radioSetting))
					continue;
				radioComp.UpdateChannelServer(1);
				radioComp.UpdateFrequncyServer(radioSetting.m_sFreq);
				restored.Insert(radio);
				//Volume and ear are put back by the client itself (SCR_PlayerController.RestoreLocalRadioSettings).
			}
		}
		
		SCR_Faction playerFaction = SCR_Faction.Cast(SCR_FactionManager.Cast(GetGame().GetFactionManager()).GetPlayerFaction(playerId));
		SCR_AIGroup playersGroup = GetPlayerGroup(playerId);
		for (int i = 0; i < playerController.m_aRadios.Count(); i++)
		{
			bool m_bFrequencyFound = false;
			IEntity radioEntity = playerController.m_aRadios.Get(i);
			if (!radioEntity || restored.Contains(radioEntity))
				continue;
			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radioEntity.FindComponent(CVON_RadioComponent));
			if (!radioComp)
				continue;
			if (!playersGroup || !playerFaction)
			{
				radioComp.UpdateChannelServer(1);
				radioComp.UpdateFrequncyServer("55500");
				continue;
			}
			switch (radioComp.m_eRadioType)
			{
				case CVON_ERadioType.SHORT:
				{
					radioComp.UpdateChannelServer(1);
					radioComp.UpdateFrequncyServer(playersGroup.GetRadioFrequency().ToString());
					break;
				}
				case CVON_ERadioType.LONG:
				{
					radioComp.UpdateChannelServer(1);
					radioComp.UpdateFrequncyServer(playerFaction.GetFactionRadioFrequency().ToString());
					break;
				}
			}
			
		}
	}
}