[BaseContainerProps()]
class CVON_GroupFrequencyContainer
{
	[Attribute()] ref array<string> m_aGroupNames;
	[Attribute()] ref array<string> m_aSRFrequencies;
	[Attribute()] ref array<string> m_aLRFrequencies;

	//Bifrost VON: what radios call this group's nets ("1 PLT NET"); empty uses the first group name
	[Attribute("", desc: "Name radios show for this group's frequencies, e.g. 1 PLT NET. Empty: the first group name.")]
	string m_sNetName;
}

[BaseContainerProps()]
modded class SCR_FactionCallsignInfo
{
	[Attribute()] ref array<ref CVON_GroupFrequencyContainer> m_aGroupFrequencyOverrides;

	[Attribute("", desc: "Frequency this faction's vehicle radios start on, and return to after a key fill. Empty: the frequency config's vehicle frequency.")]
	string m_sVehicleFrequency;
}