# Issues, Windows

## Custom Titlebar

https://kubyshkin.name/posts/win32-window-custom-title-bar-caption/

Window painting works like this:
- Windows sends a WM_PAINT message to ask the window to redraw its client area
- Windows sends a WM_NCPAINT message to ask the window to redraw its NON-client (NC) area.

However, in Vista, they changed non-client rendering to be done by the system on the GPU.
A new way was introduced.

Instead of overriding WM_NCPAINT, you override WM_NCCALCSIZE.
Here's how NCCALCSIZE works:
- Windows gives you a handle to the rectangle that represents the client area (in lparam). This starts as the entire window.
- You are free to shrink this window to make room for the frame.
- Windows uses the modified handle afterwards as the client rect.
- Note that this call also does other stuff; this logic only applies when wparam is true.

In reality, NCCALCSIZE gives you a handle to a struct:
	rgrc[3]: three rectangles. On input:
		rgrc[0]: proposed new coordinates of the whole window
		rgrc[1]: old coordinates of the whole window
		rgrc[2]: old coordinates of the client area
	rgrc[3] is also used to return output:
		rgrc[0]: new client rectangle
		rgrc[1]: "valid destination rectangle"
		rgrc[2]: "valid source rectangle"
	the struct also has another field, lppos, which is the new size and position values but in a different format.

How windows resizes the window depends on the return value.
Normally, you can return 0. This means
- the new client rectangle is in `rgrc[0]`
- None of the other rgrc fields are used
- The pixels in the old client area are saved, and draw in the upper-left corner of the new client area.

By changing the return values, you can ask windows to take what's already in the client area and align it various ways.
The most sophisticated way is with WVR_VALIDRECTS which uses `rgrc[1]` and `rgrc[2]` to save a different portion of the client area.
Lucky for us, we don't care about any of that shit.

In order to trigger NCCALCSIZE, we gotta move the window (or resize it). To do this, we call setwindowpos (and just set the window to its current position).
This forces a message to trigger the resize.

OK, cool.

In vista, you then make a call (DwmExtendFrameIntoClientArea) to allow windows to draw the frame on top of your
newly-extended client area. Note that in vista, the close/minimize/fullscreen buttons and frame are still drawn by the OS, 
but when the client area is extended, app title and icon are NOT rendered. Your app can draw on top of these though.

HOWEVER

In windows 10, there's not really any "Frame" anymore. This is a problem if you want to try and resize a window- What do you click on?

Well, actually, there is! All Win10 windows have a transparent frame that extends a few pixels
beyond the client area on the sides and bottom. It's included in the frame on the top.
However, this was implemented seperately from the frame in Vista, and you don't have control over it.

This means that on windows 10, you should NOT call the dwm frame extending function, since
the "frame" is outside your window and Windows handles it for you.

OK, great. This is getting close but there are still problems
- You need the following window flags on creation:
	- WS_THICKFRAME, for resizing
	- WS_SYSMENU, for Win+Left and Win+Right snapping support
	- WS_MAXIMIZEBOX, for win+up and snapping using the top edge
	- WS_MINIMIZEBOX, for win+down

If you're using that dwm frame extending call, that combo of flags will bring back the header.
So, don't use it!

Thus, there is now an incompatibility between windows versions:
- Pre-vista: Draw directly on non-client area with WM_NCPAINT
- Vista/Win7: Draw on nonclient area by extending client area, and if you want it to look vista-y, extending the non-client area back into the newly extended client area
- Windows 8.1: same as win10 I think?
- Windows 10: Draw on nonclient area by extending client area, WITHOUT extending nonclient area.

## DPI
- Windows Vista: System-wide DPI; if there are two monitors on the system with different DPI, your app will only look good on one of them
- Windows 8.1: Per-monitor DPI V1, UI elements do not automatically scale
- Windows 10: Per-monitor DPI V2, UI elements autoscale.

The normal version of this app targets Per-monitor DPI V1 and as such requires windows 8.1 at a minimum.