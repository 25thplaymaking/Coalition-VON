//A knob on a radio picture that turns to its positions. The knob is its own image over the radio; turning it
//rotates that image around the knob's centre, easing to the new position like a switch clicking into place.
//Angles are where the knob's pointer points, in degrees anticlockwise from 3 o'clock (as read off the
//markings printed around the real knob).
class CVON_RadioKnob
{
	static const float TURN_RATE = 16;		//how quickly it eases to a new position (per second)

	protected ImageWidget m_wImage;
	protected ImageWidget m_wShadow;		//optional shadow picture, offset on screen, turning with the knob
	protected float m_fArtAngle;			//where the pointer points in the knob's picture
	protected ref array<float> m_aDetents;	//pointer angle of each position
	protected float m_fAngle;
	protected int m_iDetent = -1;

	//pivotX, pivotY: the knob's centre as a fraction of the image's width and height
	void CVON_RadioKnob(ImageWidget image, float pivotX, float pivotY, float artAngle, array<float> detents)
	{
		m_wImage = image;
		m_fArtAngle = artAngle;
		m_aDetents = detents;
		if (m_wImage)
			m_wImage.SetPivot(pivotX, pivotY);
	}

	//A shadow drawn the same size as the knob picture, placed a little below it: turns about its own centre
	void SetShadow(ImageWidget shadow, float pivotX, float pivotY)
	{
		m_wShadow = shadow;
		if (m_wShadow)
			m_wShadow.SetPivot(pivotX, pivotY);
	}

	//Turn to a position. The first call places the knob without turning (the menu opens showing it where it is).
	//Returns true when the knob starts turning (for the click sound).
	bool SetDetent(int detent)
	{
		if (detent < 0 || detent >= m_aDetents.Count() || detent == m_iDetent)
			return false;
		bool first = m_iDetent < 0;
		m_iDetent = detent;
		if (!first)
			return true;
		m_fAngle = m_aDetents[detent];
		Apply();
		return false;
	}

	void Update(float tDelta)
	{
		if (m_iDetent < 0)
			return;
		float target = m_aDetents[m_iDetent];
		float diff = target - m_fAngle;
		if (diff == 0)
			return;
		if (Math.AbsFloat(diff) < 0.1)
			m_fAngle = target;
		else
			m_fAngle += diff * Math.Min(1, tDelta * TURN_RATE);
		Apply();
	}

	protected void Apply()
	{
		//Widget rotation runs clockwise on screen; knob angles run anticlockwise
		if (m_wImage)
			m_wImage.SetRotation(m_fArtAngle - m_fAngle);
		if (m_wShadow)
			m_wShadow.SetRotation(m_fArtAngle - m_fAngle);
	}

	int GetDetent()
	{
		return m_iDetent;
	}

	float GetAngle()
	{
		return m_fAngle;
	}

	bool IsTurning()
	{
		return m_iDetent >= 0 && m_fAngle != m_aDetents[m_iDetent];
	}
}

//Whether a key on the radio picture is being held down with the left mouse button, for keys that do something else
//when held (the R-187's red key: held, it switches the radio on or off)
class CVON_HoldKeyHandler : ScriptedWidgetEventHandler
{
	bool m_bDown;

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_bDown = true;
		return false;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_bDown = false;
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_bDown = false;
		return false;
	}
}