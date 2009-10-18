INT
Test_NtUserRedrawWindow(PTESTINFO pti)
{
	HWND hWnd;
	RECT rect;

	hWnd = CreateWindowA("BUTTON",
	                     "Test",
	                     BS_PUSHBUTTON | WS_VISIBLE,
	                     0,
	                     0,
	                     50,
	                     30,
	                     NULL,
	                     NULL,
	                     g_hInstance,
	                     0);
	ASSERT(hWnd);

	rect.left = 0;
	rect.top = 0;
	rect.right = 10;
	rect.bottom = 10;

	TEST(NtUserRedrawWindow(hWnd, &rect, NULL, RDW_VALIDATE) == TRUE);

	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}
