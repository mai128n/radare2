/* radare - LGPL - Copyright 2009-2019 - pancake */

#include <r_cons.h>
#include <string.h>
#if __UNIX__
#include <errno.h>
#endif

#define I r_cons_singleton ()

// TODO: Support binary, use RBuffer and remove globals
static char *readbuffer = NULL;
static int readbuffer_length = 0;
static bool bufactive = true;

#if 0
//__UNIX__
#include <poll.h>
static int __is_fd_ready(int fd) {
	fd_set rfds;
	struct timeval tv;
	if (fd==-1)
		return 0;
	FD_ZERO (&rfds);
	FD_SET (fd, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 1;
	if (select (1, &rfds, NULL, NULL, &tv) == -1)
		return 0;
	return 1;
	return !FD_ISSET (0, &rfds);
}
#endif

R_API int r_cons_controlz(int ch) {
#if __UNIX__
	if (ch == 0x1a) {
		r_cons_show_cursor (true);
		r_cons_enable_mouse (false);
		r_sys_stop ();
		return 0;
	}
#endif
	return ch;
}

// 96 - wheel up
// 97 - wheel down
// 95 - mouse up
// 92 - mouse down
static int __parseMouseEvent() {
	char xpos[32];
	char ypos[32];
	int ch = r_cons_readchar ();
	int ch2 = r_cons_readchar ();

	// [32M - mousedown
	// [35M - mouseup
	if (ch2 == ';') {
		int i;
		// read until next ;
		for (i = 0; i < sizeof (xpos); i++) {
			char ch = r_cons_readchar ();
			if (ch == ';' || ch == 'M') {
				break;
			}
			xpos[i] = ch;
		}
		xpos[i] = 0;
		for (i = 0; i < sizeof (ypos); i++) {
			char ch = r_cons_readchar ();
			if (ch == ';' || ch == 'M') {
				break;
			}
			ypos[i] = ch;
		}
		ypos[i] = 0;
		r_cons_set_click (atoi (xpos), atoi (ypos));
		ch = r_cons_readchar ();
		// ignored
		int ch = r_cons_readchar ();
		if (ch == 27) {
			ch = r_cons_readchar (); // '['
		}
		if (ch == '[') {
			do {
				ch = r_cons_readchar (); // '3'
			} while (ch != 'M');
		}
	}
	return 0;
}

R_API int r_cons_arrow_to_hjkl(int ch) {
#if __WINDOWS__
	return ch;
#endif
	I->mouse_event = 0;
	/* emacs */
	switch ((ut8)ch) {
	case 0xc3: r_cons_readchar (); ch='K'; break; // emacs repag (alt + v)
	case 0x16: ch='J'; break; // emacs avpag (ctrl + v)
	case 0x10: ch='k'; break; // emacs up (ctrl + p)
	case 0x0e: ch='j'; break; // emacs down (ctrl + n)
	case 0x06: ch='l'; break; // emacs right (ctrl + f)
	case 0x02: ch='h'; break; // emacs left (ctrl + b)
	}
	if (ch != 0x1b) {
		return ch;
	}
	ch = r_cons_readchar ();
	if (!ch) {
		return 0;
	}
	switch (ch) {
	case 0x1b:
		ch = 'q'; // XXX: must be 0x1b (R_CONS_KEY_ESC)
		break;
	case 0x4f: // function keys from f1 to f4
		ch = r_cons_readchar ();
#if defined(__HAIKU__)
		/* Haiku't don use the '[' char for function keys */
		if (ch > 'O') {/* only in f1..f12 function keys */
			ch = 0xf1 + (ch&0xf);
			break;
		}
	case '[': // 0x5b function keys (2)
		/* Haiku need ESC + [ for PageUp and PageDown  */
		if (ch < 'A' || ch == '[') {
			ch = r_cons_readchar ();
		}
#else
		ch = 0xf1 + (ch & 0xf);
		break;
	case '[': // function keys (2)
		ch = r_cons_readchar ();
#endif
		switch (ch) {
		case '<':
			{
				char pos[8] = {0};
				int p = 0;
				int x = 0;
				int y = 0;
				int sc = 0;
				do {
					ch = r_cons_readchar ();
					eprintf ( "%c", ch);
					if (sc > 0) {
						if (ch >= '0'&& ch <= '9') {
							pos[p++] = ch;
						}
					}
					if (ch == ';') {
						if (sc == 1) {
							pos[p++] = 0;
							x = atoi (pos);
						}
						sc++;
						p = 0;
					}	
				} while (ch != 'M' && ch != 'm');
				pos[p++] = 0;
				y = atoi (pos);
				// M is mouse down , m is mouse up
				if (ch == 'M') {
					r_cons_set_click (x, y);
				}
			}
			return 0;
			break;
		case '[':
			ch = r_cons_readchar ();
			switch (ch) {
			case '2': ch = R_CONS_KEY_F11; break;
			case 'A': ch = R_CONS_KEY_F1; break;
			case 'B': ch = R_CONS_KEY_F2; break;
			case 'C': ch = R_CONS_KEY_F3; break;
			case 'D': ch = R_CONS_KEY_F4; break;
			}
			break;
		case '9':
			// handle mouse wheel
	//		__parseWheelEvent();
			ch = r_cons_readchar ();
			// 6 is up
			// 7 is down
			if (ch == '6') {
				ch = 'k';
			} else if (ch == '7') {
				ch = 'j';
			} else {
				// unhandled case
				ch = 0;
			}
			int ch2;
			do {
				ch2 = r_cons_readchar ();
			} while (ch2 != 'M');
			break;
		case '3':
			// handle mouse down /up events (35 vs 32)
			__parseMouseEvent();
			return 0;
			break;
		case '2':
			ch = r_cons_readchar ();
			switch (ch) {
			case 0x7e:
				ch = R_CONS_KEY_F12;
				break;
			default:
				r_cons_readchar ();
				switch (ch) {
				case '0': ch = R_CONS_KEY_F9; break;
				case '1': ch = R_CONS_KEY_F10; break;
				case '3': ch = R_CONS_KEY_F11; break;
				}
				break;
			}
			break;
		case '1':
			ch = r_cons_readchar ();
			switch (ch) {
			// Support st/st-256color term and others
			// for shift+arrows
			case ';': // arrow+mod
				ch = r_cons_readchar ();
				switch (ch) {
				case '2': // arrow+shift
					ch = r_cons_readchar ();
					switch (ch) {
					case 'A': ch = 'K'; break;
					case 'B': ch = 'J'; break;
					case 'C': ch = 'L'; break;
					case 'D': ch = 'H'; break;
					}
					break;
				// add other modifiers
				}
				break;
			case ':': // arrow+shift
				ch = r_cons_readchar ();
				ch = r_cons_readchar ();
				switch (ch) {
				case 'A': ch = 'K'; break;
				case 'B': ch = 'J'; break;
				case 'C': ch = 'L'; break;
				case 'D': ch = 'H'; break;
				}
				break;
/*
			case '1': ch = R_CONS_KEY_F1; break;
			case '2': ch = R_CONS_KEY_F2; break;
			case '3': ch = R_CONS_KEY_F3; break;
			case '4': ch = R_CONS_KEY_F4; break;
*/
			case '5':
				r_cons_readchar ();
				ch = 0xf5;
				break;
			case '6':
				r_cons_readchar ();
				ch = 0xf7;
				break;
			case '7':
				r_cons_readchar ();
				ch = 0xf6;
				break;
			case '8':
				r_cons_readchar ();
				ch = 0xf7;
				break;
			case '9':
				r_cons_readchar ();
				ch = 0xf8;
				break;
			} // F9-F12 not yet supported!!
			break;
		case '5': ch = 'K'; r_cons_readchar (); break; // repag
		case '6': ch = 'J'; r_cons_readchar (); break; // avpag
		/* arrow keys */
		case 'A': ch = 'k'; break; // up
		case 'B': ch = 'j'; break; // down
		case 'C': ch = 'l'; break; // right
		case 'D': ch = 'h'; break; // left
		// Support rxvt-unicode term for shift+arrows
		case 'a': ch = 'K'; break; // shift+up
		case 'b': ch = 'J'; break; // shift+down
		case 'c': ch = 'L'; break; // shift+right
		case 'd': ch = 'H'; break; // shift+left
		case 'M': ch = __parseMouseEvent (); break;
		}
		break;
	}
	return ch;
}

// XXX no control for max length here?!?!
R_API int r_cons_fgets(char *buf, int len, int argc, const char **argv) {
#define RETURN(x) { ret=x; goto beach; }
	RCons *cons = r_cons_singleton ();
	int ret = 0, color = cons->context->pal.input && *cons->context->pal.input;
	if (cons->echo) {
		r_cons_set_raw (false);
		r_cons_show_cursor (true);
	}
#if 0
	int mouse = r_cons_enable_mouse (false);
	r_cons_enable_mouse (false);
	r_cons_flush ();
#endif
	errno = 0;
	if (cons->user_fgets) {
		RETURN (cons->user_fgets (buf, len));
	}
	printf ("%s", cons->line->prompt);
	fflush (stdout);
	*buf = '\0';
	if (color) {
		const char *p = cons->context->pal.input;
		int len = p? strlen (p): 0;
		if (len > 0) {
			fwrite (p, len, 1, stdout);
		}
		fflush (stdout);
	}
	if (!fgets (buf, len, cons->fdin)) {
		if (color) {
			printf (Color_RESET);
			fflush (stdout);
		}
		RETURN (-1);
	}
	if (feof (cons->fdin)) {
		if (color) {
			printf (Color_RESET);
		}
		RETURN (-2);
	}
	buf[strlen (buf)-1] = '\0';
	if (color) {
		printf (Color_RESET);
	}
	ret = strlen (buf);
beach:
	//r_cons_enable_mouse (mouse);
	return ret;
}

R_API int r_cons_any_key(const char *msg) {
	if (msg && *msg) {
		r_cons_printf ("\n-- %s --\n", msg);
	} else {
		r_cons_print ("\n--press any key--\n");
	}
	r_cons_flush ();
	return r_cons_readchar ();
	//r_cons_strcat ("\x1b[2J\x1b[0;0H"); // wtf?
}

extern void resizeWin(void);

#if __WINDOWS__
static int __cons_readchar_w32 (ut32 usec) {
	int ch = 0;
	BOOL ret;
	BOOL bCtrl = FALSE;
	DWORD mode, out;
	HANDLE h;
	INPUT_RECORD irInBuf[128];
	int i, o;
	bool resize = false;
	void *bed;
	h = GetStdHandle (STD_INPUT_HANDLE);
	GetConsoleMode (h, &mode);
	SetConsoleMode (h, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
do_it_again:
	bed = r_cons_sleep_begin ();
	if (usec) {
		if (WaitForSingleObject (h, usec) == WAIT_TIMEOUT) {
			r_cons_sleep_end (bed);
			return -1;
		}
	}
	ret = ReadConsoleInput (h, irInBuf, 128, &out);
	r_cons_enable_mouse (true);
	r_cons_sleep_end (bed);
	if (ret) {
		for (i = 0; i < out; i++) {
			if (irInBuf[i].EventType == MOUSE_EVENT) {
				if (irInBuf[i].Event.MouseEvent.dwEventFlags == MOUSE_WHEELED) {
					if (irInBuf[i].Event.MouseEvent.dwButtonState & 0xFF000000) {
						ch = bCtrl ? 'J' : 'j';
					} else {
						ch = bCtrl ? 'K' : 'k';
					}
					break;
				}
				switch (irInBuf[i].Event.MouseEvent.dwButtonState) {
				case FROM_LEFT_1ST_BUTTON_PRESSED:
					r_cons_enable_mouse (false);
				} // TODO: Handle more buttons?
			}
			if (irInBuf[i].EventType == KEY_EVENT) {
				if (irInBuf[i].Event.KeyEvent.bKeyDown) {
					ch = irInBuf[i].Event.KeyEvent.uChar.AsciiChar;
					bCtrl=irInBuf[i].Event.KeyEvent.dwControlKeyState & 8;
					if (irInBuf[i].Event.KeyEvent.uChar.AsciiChar == 0) {
						ch = 0;
						switch (irInBuf[i].Event.KeyEvent.wVirtualKeyCode) {
						case VK_DOWN: // key down
							ch = bCtrl ? 'J' : 'j';
							break;
						case VK_RIGHT: // key right
							ch = bCtrl ? 'L' : 'l';
							break;
						case VK_UP: // key up
							ch = bCtrl ? 'K' : 'k';
							break;
						case VK_LEFT: // key left
							ch = bCtrl ? 'H' : 'h';
							break;
						case VK_PRIOR: // key home
							ch = 'K';
							break;
						case VK_NEXT: // key end
							ch = 'J';
							break;
						case VK_F1:
							ch = R_CONS_KEY_F1;
							break;
						case VK_F2:
							ch = R_CONS_KEY_F2;
							break;
						case VK_F3:
							ch = R_CONS_KEY_F3;
							break;
						case VK_F4:
							ch = R_CONS_KEY_F4;
							break;
						case VK_F5:
							ch = bCtrl ? 0xcf5 : R_CONS_KEY_F5;
							break;
						case VK_F6:
							ch = R_CONS_KEY_F6;
							break;
						case VK_F7:
							ch = R_CONS_KEY_F7;
							break;
						case VK_F8:
							ch = R_CONS_KEY_F8;
							break;
						case VK_F9:
							ch = R_CONS_KEY_F9;
							break;
						case VK_F10:
							ch = R_CONS_KEY_F10;
							break;
						case VK_F11:
							ch = R_CONS_KEY_F11;
							break;
						case VK_F12:
							ch = R_CONS_KEY_F12;
							break;
						default:
							ch = 0;
							break;
						}
					}
				}
			}
			if (irInBuf[i].EventType == WINDOW_BUFFER_SIZE_EVENT) {
				resize = true;
			}
		}
		if (resize) {
			resizeWin ();
			resize = false;
		}
	}
	FlushConsoleInputBuffer (h);
	if (ch == 0) {
		goto do_it_again;
	}
	SetConsoleMode (h, mode);
	/*r_cons_gotoxy (1, 2);
	r_cons_printf ("\n");
	r_cons_printf ("| buf = %x |\n", ch);
	r_cons_printf ("\n");
	r_cons_flush ();
	r_sys_sleep (1);*/
	return  ch;
}
#endif

R_API int r_cons_readchar_timeout(ut32 usec) {
#if __UNIX__
	struct timeval tv;
	fd_set fdset, errset;
	FD_ZERO (&fdset);
	FD_ZERO (&errset);
	FD_SET (0, &fdset);
	tv.tv_sec = 0; // usec / 1000;
	tv.tv_usec = 1000 * usec;
	r_cons_set_raw (1);
	if (select (1, &fdset, NULL, &errset, &tv) == 1) {
		return r_cons_readchar ();
	}
	r_cons_set_raw (0);
	// timeout
	return -1;
#else
	return  __cons_readchar_w32 (usec);
#endif
}

R_API bool r_cons_readpush(const char *str, int len) {
	char *res = (len + readbuffer_length > 0) ? realloc (readbuffer, len + readbuffer_length) : NULL;
	if (res) {
		readbuffer = res;
		memmove (readbuffer + readbuffer_length, str, len);
		readbuffer_length += len;
		return true;
	}
	return false;
}

R_API void r_cons_readflush() {
	R_FREE (readbuffer);
	readbuffer_length = 0;
}

R_API void r_cons_switchbuf(bool active) {
	bufactive = active;
}

#if !__WINDOWS__
extern volatile sig_atomic_t sigwinchFlag;
#endif

R_API int r_cons_readchar() {
	void *bed;
	char buf[2];
	buf[0] = -1;
	if (readbuffer_length > 0) {
		int ch = *readbuffer;
		readbuffer_length--;
		memmove (readbuffer, readbuffer + 1, readbuffer_length);
		return ch;
	}
#if __WINDOWS__
	return __cons_readchar_w32 (0);
#else
	r_cons_set_raw (1);
	bed = r_cons_sleep_begin ();

	// Blocks until either stdin has something to read or a signal happens.
	// This serves to check if the terminal window was resized. It avoids the race
	// condition that could happen if we did not use pselect or select in case SIGWINCH
	// was handled immediately before the blocking call (select or read). The race is
	// prevented from happening by having SIGWINCH blocked process-wide except for in
	// pselect (that is what pselect is for).
	fd_set readfds;
	sigset_t sigmask;
	FD_ZERO (&readfds);
	FD_SET (STDIN_FILENO, &readfds);
	r_signal_sigmask (0, NULL, &sigmask);
	sigdelset (&sigmask, SIGWINCH);
	while (pselect (STDIN_FILENO + 1, &readfds, NULL, NULL, NULL, &sigmask) == -1) {
		if (errno == EBADF) {
			eprintf ("r_cons_readchar (): EBADF\n");
			return -1;
		}
		if (sigwinchFlag) {
			sigwinchFlag = 0;
			resizeWin ();
		}
	}

	ssize_t ret = read (STDIN_FILENO, buf, 1);
	r_cons_sleep_end (bed);
	if (ret != 1) {
		return -1;
	}
	if (bufactive) {
		r_cons_set_raw (0);
	}
	return r_cons_controlz (buf[0]);
#endif
}

R_API bool r_cons_yesno(int def, const char *fmt, ...) {
	va_list ap;
	ut8 key = (ut8)def;
	va_start (ap, fmt);

	if (!r_cons_is_interactive ()) {
		va_end (ap);
		return def == 'y';
	}
	vfprintf (stderr, fmt, ap);
	va_end (ap);
	fflush (stderr);
	r_cons_set_raw (true);
	(void)read (0, &key, 1);
	write (2, " ", 1);
	write (2, &key, 1);
	write (2, "\n", 1);
	if (key == 'Y') {
		key = 'y';
	}
	r_cons_set_raw (false);
	if (key == '\n' || key == '\r') {
		key = def;
	}
	return key == 'y';
}

R_API char *r_cons_password(const char *msg) {
	int i = 0;
	char buf[256] = {0};
	printf ("\r%s", msg);
	fflush (stdout);
	r_cons_set_raw (1);
#if __UNIX__
	RCons *a = r_cons_singleton ();
	a->term_raw.c_lflag &= ~(ECHO | ECHONL);
	// //  required to make therm/iterm show the key
	// // cannot read when enabled in this way
	// a->term_raw.c_lflag |= ICANON;
	tcsetattr (0, TCSADRAIN, &a->term_raw);
	signal (SIGTSTP, SIG_IGN);
#endif
	while (i < sizeof (buf) - 1) {
		int ch = r_cons_readchar ();
		if (ch == 127) { // backspace
			if (i < 1) {
				break;
			}
			i--;
			continue;
		}
		if (ch == '\r' || ch == '\n') {
			break;
		}
		buf[i++] = ch;
	}
	buf[i] = 0;
	r_cons_set_raw (0);
	printf ("\n");
#if __UNIX__
	signal (SIGTSTP, SIG_DFL);
#endif
	return strdup (buf);
}

R_API char *r_cons_input(const char *msg) {
	char *oprompt = r_line_get_prompt ();
	if (!oprompt) {
		return NULL;
	}
	char buf[1024];
	if (msg) {
		r_line_set_prompt (msg);
	} else {
		r_line_set_prompt ("");
	}
	buf[0] = 0;
	r_cons_fgets (buf, sizeof (buf), 0, NULL);
	r_line_set_prompt (oprompt);
	free (oprompt);
	return strdup (buf);
}
