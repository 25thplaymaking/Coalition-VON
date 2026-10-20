//------------------------------------------------------------------------------------------------
// Features addons can test for with #ifdef, so they still compile against older CVON builds
//------------------------------------------------------------------------------------------------

//! Listening to another player's radios: SCR_PlayerController.SetRadioListenTarget / GetRadioListenTarget,
//! CVON_VONGameModeComponent.CanListenToRadios (override it to allow someone)
#define CVON_RADIO_LISTEN
