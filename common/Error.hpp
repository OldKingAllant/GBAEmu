namespace GBA::error {
	void Assert(bool value, const char* message);
	void DebugBreak();
	void BailOut(bool bad);
}