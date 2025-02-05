/* radare - LGPL - Copyright 2013-2019 - pancake */

#include <r_anal.h>

#define DB a->sdb_hints
#define setf(x,...) snprintf(x,sizeof(x)-1,##__VA_ARGS__)

R_API void r_anal_hint_clear(RAnal *a) {
	sdb_reset (a->sdb_hints);
}

R_API void r_anal_hint_del(RAnal *a, ut64 addr, int size) {
	char key[128];
	if (size > 1) {
		eprintf ("TODO: r_anal_hint_del: in range\n");
	} else {
		setf (key, "hint.0x%08"PFMT64x, addr);
		sdb_unset (a->sdb_hints, key, 0);
	}
}

static void unsetHint(RAnal *a, const char *type, ut64 addr) {
	char key[128];
	setf (key, "hint.0x%08"PFMT64x, addr);
	int idx = sdb_array_indexof (DB, key, type, 0);
	if (idx != -1) {
		sdb_array_delete (DB, key, idx, 0);
		sdb_array_delete (DB, key, idx, 0);
	}
}

static void setHint(RAnal *a, const char *type, ut64 addr, const char *s, ut64 ptr) {
	char key[128], val[128], *nval = NULL;
	setf (key, "hint.0x%08"PFMT64x, addr);
	int idx = sdb_array_indexof (DB, key, type, 0);
	if (s) {
		nval = sdb_encode ((const ut8*)s, -1);
	} else {
		nval = sdb_itoa (ptr, val, 16);
	}
	if (idx != -1) {
		if (!s) {
			nval = sdb_itoa (ptr, val, 16);
		}
		sdb_array_set (DB, key, idx + 1, nval, 0);
	} else {
		sdb_array_push (DB, key, nval, 0);
		sdb_array_push (DB, key, type, 0);
	}
	if (s) {
		free (nval);
	}
}

R_API void r_anal_hint_set_offset(RAnal *a, ut64 addr, const char* typeoff) {
	setHint (a, "Offset:", addr, r_str_trim_ro (typeoff), 0);
}

R_API void r_anal_hint_set_nword(RAnal *a, ut64 addr, int nword) {
	setHint (a, "nword:", addr, NULL, nword);
}

R_API void r_anal_hint_set_jump(RAnal *a, ut64 addr, ut64 ptr) {
	setHint (a, "jump:", addr, NULL, ptr);
}

R_API void r_anal_hint_set_newbits(RAnal *a, ut64 addr, int bits) {
	setHint (a, "Bits:", addr, NULL, bits);
}

// TODO: add helpers for newendian and newbank

R_API void r_anal_hint_set_fail(RAnal *a, ut64 addr, ut64 ptr) {
	setHint (a, "fail:", addr, NULL, ptr);
}

R_API void r_anal_hint_set_high(RAnal *a, ut64 addr) {
	setHint (a, "high:", addr, NULL, 1);
}

R_API void r_anal_hint_set_immbase(RAnal *a, ut64 addr, int base) {
	if (base) {
		setHint (a, "immbase:", addr, NULL, (ut64)base);
	} else {
		unsetHint (a, "immbase:", addr);
	}
}

R_API void r_anal_hint_set_pointer(RAnal *a, ut64 addr, ut64 ptr) {
	setHint (a, "ptr:", addr, NULL, ptr);
}

R_API void r_anal_hint_set_ret(RAnal *a, ut64 addr, ut64 val) {
	setHint (a, "ret:", addr, NULL, val);
}

R_API void r_anal_hint_set_arch(RAnal *a, ut64 addr, const char *arch) {
	setHint (a, "arch:", addr, r_str_trim_ro (arch), 0);
}

R_API void r_anal_hint_set_syntax(RAnal *a, ut64 addr, const char *syn) {
	setHint (a, "Syntax:", addr, syn, 0);
}

R_API void r_anal_hint_set_opcode(RAnal *a, ut64 addr, const char *opcode) {
	setHint (a, "opcode:", addr, r_str_trim_ro (opcode), 0);
}

R_API void r_anal_hint_set_esil(RAnal *a, ut64 addr, const char *esil) {
	setHint (a, "esil:", addr, r_str_trim_ro (esil), 0);
}

R_API void r_anal_hint_set_type (RAnal *a, ut64 addr, int type) {
	setHint (a, "type:", addr, NULL, (ut64)type);
}

R_API void r_anal_hint_set_bits(RAnal *a, ut64 addr, int bits) {
	setHint (a, "bits:", addr, NULL, bits);
	if (a && a->hint_cbs.on_bits) {
		a->hint_cbs.on_bits (a, addr, bits, true);
	}
	a->merge_hints = true;
}

R_API void r_anal_hint_set_size(RAnal *a, ut64 addr, int size) {
	setHint (a, "size:", addr, NULL, size);
}

R_API void r_anal_hint_set_stackframe(RAnal *a, ut64 addr, ut64 size) {
	setHint (a, "Frame:", addr, NULL, size);
}

R_API void r_anal_hint_unset_size(RAnal *a, ut64 addr) {
	unsetHint(a, "size:", addr);
}

R_API void r_anal_hint_unset_bits(RAnal *a, ut64 addr) {
	unsetHint(a, "bits:", addr);
	if (a && a->hint_cbs.on_bits) {
		a->hint_cbs.on_bits (a, addr, 0, false);
	}
	a->merge_hints = true;
}

R_API void r_anal_hint_unset_esil(RAnal *a, ut64 addr) {
	unsetHint(a, "esil:", addr);
}

R_API void r_anal_hint_unset_opcode(RAnal *a, ut64 addr) {
	unsetHint(a, "opcode:", addr);
}

R_API void r_anal_hint_unset_high(RAnal *a, ut64 addr) {
	unsetHint(a, "high:", addr);
}

R_API void r_anal_hint_unset_arch(RAnal *a, ut64 addr) {
	unsetHint(a, "arch:", addr);
}

R_API void r_anal_hint_unset_nword(RAnal *a, ut64 addr) {
	unsetHint(a, "nword:", addr);
}

R_API void r_anal_hint_unset_syntax(RAnal *a, ut64 addr) {
	unsetHint(a, "Syntax:", addr);
}

R_API void r_anal_hint_unset_pointer(RAnal *a, ut64 addr) {
	unsetHint(a, "ptr:", addr);
}

R_API void r_anal_hint_unset_ret(RAnal *a, ut64 addr) {
	unsetHint(a, "ret:", addr);
}

R_API void r_anal_hint_unset_offset(RAnal *a, ut64 addr) {
	unsetHint (a, "Offset:", addr);
}

R_API void r_anal_hint_unset_jump(RAnal *a, ut64 addr) {
	unsetHint (a, "jump:", addr);
}

R_API void r_anal_hint_unset_fail(RAnal *a, ut64 addr) {
	unsetHint (a, "fail:", addr);
}

R_API void r_anal_hint_unset_type (RAnal *a, ut64 addr) {
	unsetHint (a, "type:", addr);
}

R_API void r_anal_hint_unset_stackframe(RAnal *a, ut64 addr) {
	unsetHint (a, "Frame:", addr);
}

R_API void r_anal_hint_free(RAnalHint *h) {
	if (h) {
		free (h->arch);
		free (h->esil);
		free (h->opcode);
		free (h->syntax);
		free (h->offset);
		free (h);
	}
}

R_API int r_anal_hint_get_bits_at(RAnal *a, ut64 addr, const char *str) {
	if (a->opt.ignbithints) {
		return 0;
	}
	char *r, *nxt, *nxt2;
	char *s = strdup (str);
	int token = 0, bits = 0;
	if (!s) {
		return 0;
	}
	token = *s;
	for (r = s; ; r = nxt2) {
		r = sdb_anext (r, &nxt);
		if (!nxt) {
			break;
		}
		sdb_anext (nxt, &nxt2); // tokenize value
		if (token) {
			switch (token) {
			case 'b':
				bits = sdb_atoi (nxt);
				break;
			}
		}
		if (!nxt || !nxt2) {
			break;
		}
		token = *nxt2;
	}
	free (s);
	return bits;
}

// TODO: missing to_string ()
R_API RAnalHint *r_anal_hint_from_string(RAnal *a, ut64 addr, const char *str) {
	char *r, *nxt, *nxt2;
	int token = 0;
	RAnalHint *hint = R_NEW0 (RAnalHint);
	if (!hint) {
		return NULL;
	}
	hint->jump = UT64_MAX;
	hint->fail = UT64_MAX;
	hint->ret = UT64_MAX;
	hint->stackframe = UT64_MAX;
	char *s = strdup (str);
	if (!s) {
		free (hint);
		return NULL;
	}
	hint->addr = addr;
	token = *s;
	for (r = s; ; r = nxt2) {
		r = sdb_anext (r, &nxt);
		if (!nxt) {
			break;
		}
		sdb_anext (nxt, &nxt2); // tokenize value
		if (token) {
			switch (token) {
			case 'i': hint->immbase = sdb_atoi (nxt); break;
			case 'j': hint->jump = sdb_atoi (nxt); break;
			case 'f': hint->fail = sdb_atoi (nxt); break;
			case 'F': hint->stackframe = sdb_atoi (nxt); break;
			case 'p': hint->ptr = sdb_atoi (nxt); break;
			case 'n': hint->nword = sdb_atoi (nxt); break;
			case 'r': hint->ret = sdb_atoi (nxt); break;
			case 'b': hint->bits = sdb_atoi (nxt); break;
			case 'B': hint->new_bits = sdb_atoi (nxt); break;
			case 's': hint->size = sdb_atoi (nxt); break;
			case 'S': hint->syntax = (char*)sdb_decode (nxt, 0); break;
			case 't': hint->type = r_num_get (NULL, nxt);  break;
			case 'o': hint->opcode = (char*)sdb_decode (nxt, 0); break;
			case 'O': hint->offset = (char*)sdb_decode (nxt, 0); break;
			case 'e': hint->esil = (char*)sdb_decode (nxt, 0); break;
			case 'a': hint->arch = (char*)sdb_decode (nxt, 0); break;
			case 'h': hint->high = sdb_atoi (nxt); break;
			}
		}
		if (!nxt || !nxt2) {
			break;
		}
		token = *nxt2;
	}
	free (s);
	return hint;
}

R_API RAnalHint *r_anal_hint_get(RAnal *a, ut64 addr) {
	char key[64];
	setf (key, "hint.0x%08"PFMT64x, addr);
	const char *s = sdb_const_get (DB, key, 0);
	return s? r_anal_hint_from_string (a, addr, s): NULL;
}
