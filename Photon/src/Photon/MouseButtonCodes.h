#pragma once

/*
Note: These mouse button codes are to be kept the same throughout different
graphics libraries. In order to convert these mouse button codes to the
library's relevant mouse button codes, a lookup table must be defined,
mapping the PT_MOUSE_BUTTON to the relevant mouse button codes.

This keeps client side mouse button codes standard which is useful in
cases such as serialisation.
*/

// From glfw3.h
#define PT_MOUSE_BUTTON_1         0
#define PT_MOUSE_BUTTON_2         1
#define PT_MOUSE_BUTTON_3         2
#define PT_MOUSE_BUTTON_4         3
#define PT_MOUSE_BUTTON_5         4
#define PT_MOUSE_BUTTON_6         5
#define PT_MOUSE_BUTTON_7         6
#define PT_MOUSE_BUTTON_8         7
#define PT_MOUSE_BUTTON_LAST      PT_MOUSE_BUTTON_8
#define PT_MOUSE_BUTTON_LEFT      PT_MOUSE_BUTTON_1
#define PT_MOUSE_BUTTON_RIGHT     PT_MOUSE_BUTTON_2
#define PT_MOUSE_BUTTON_MIDDLE    PT_MOUSE_BUTTON_3