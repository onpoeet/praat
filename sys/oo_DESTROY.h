/* oo_DESTROY.h
 *
 * Copyright (C) 1994-2007,2009-2018 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "oo_undef.h"

#define oo_SIMPLE(type,storage,x)

#define oo_ARRAY(type,storage,x,cap,n)

#define oo_SET(type,storage,x,setType)

#define oo_VECTOR(type,storage,x,min,max)  \
	{ \
		integer _min = (min); \
		NUMvector_free <type> (our x, _min); \
	}

#define oo_MATRIX(type,storage,x,row1,row2,col1,col2)  \
	{ \
		integer _row1 = (row1), _col1 = (col1); \
		NUMmatrix_free <type> (our x, _row1, _col1); \
	}

#define oo_ENUMx(kType,storage,x)

//#define oo_ENUMx_ARRAY(kType,storage,x,cap,n)

//#define oo_ENUMx_SET(kType,storage,x,setType)

//#define oo_ENUMx_VECTOR(kType,storage,x,min,max)  \
//	{ \
//		integer _min = (min); \
//		NUMvector_free <type> (our x, _min); \
//	}

#define oo_STRINGx(storage,x)  \
	if (! _thisStructCanAutodestroyItsMembers_) { \
		our x. reset (); \
	}

#define oo_STRINGx_ARRAY(storage,x,cap,n)  \
	{ \
		integer _n = (n); \
		for (int _i = 0; _i < _n; _i ++) { \
			Melder_free (our x [_i]); \
		} \
	}

#define oo_STRINGx_SET(storage,x,setType)  \
	for (int i = 0; i <= setType::MAX; i ++) \
		Melder_free (our x [i]);

#define oo_STRINGx_VECTOR(storage,x,min,max)  \
	{ \
		integer _min = (min), _max = (max); \
		if (our x) { \
			for (integer _i = _min; _i <= _max; _i ++) \
				Melder_free (our x [_i]); \
			NUMvector_free <char32*> (our x, _min); \
		} \
	}

#define oo_STRUCT(Type,x)  \
	our x. destroy ();

#define oo_STRUCT_ARRAY(Type,x,cap,n)  \
	{ \
		integer _n = (n); \
		for (int _i = 0; _i < _n; _i ++) { \
			our x [_i]. destroy (); \
		} \
	}

#define oo_STRUCT_SET(Type,x,setType)  \
	for (int _i = 0; _i <= (int) setType::MAX; _i ++) { \
		our x [_i]. destroy (); \
	}

#define oo_STRUCT_VECTOR_FROM(Type,x,min,max)  \
	{ \
		integer _min = (min), _max = (max); \
		if (our x) { \
			for (integer _i = _min; _i <= _max; _i ++) { \
				our x [_i]. destroy (); \
			} \
			NUMvector_free <struct##Type> (our x, _min); \
		} \
	}

#define oo_STRUCT_MATRIX_FROM(Type,x,row1,row2,col1,col2)  \
	{ \
		integer _row1 = (row1), _row2 = (row2), _col1 = (col1), _col2 = (col2); \
		if (our x) { \
			for (integer _irow = _row1; _irow <= _row2; _irow ++) { \
				for (integer _icol = _col1; _icol <= _col2; _icol ++) { \
					our x [_irow] [_icol]. destroy (); \
				} \
			} \
			NUMmatrix_free <struct##Type> (our x, _row1, _col1); \
		} \
	}

#define oo_OBJECT(Class,version,x)  \
	if (! _thisStructCanAutodestroyItsMembers_) { \
		x. reset (); \
	}

#define oo_COLLECTION_OF(Class,x,ItemClass,version)

#define oo_COLLECTION(Class,x,ItemClass,version)

#define oo_FILE(x)

#define oo_DIR(x)

#define oo_DEFINE_STRUCT(Struct)  \
	void struct##Struct :: destroy () { \
		constexpr bool _thisStructCanAutodestroyItsMembers_ = false; \
		(void) _thisStructCanAutodestroyItsMembers_;

#define oo_END_STRUCT(Type)  \
	}

#define oo_DEFINE_CLASS(Class,Parent)  \
	void struct##Class :: v_destroy () noexcept { \
		constexpr bool _thisStructCanAutodestroyItsMembers_ = true; \
		(void) _thisStructCanAutodestroyItsMembers_;

#define oo_END_CLASS(Class)  \
		Class##_Parent :: v_destroy (); \
	}

#define oo_FROM(from)

#define oo_ENDFROM

#define oo_DECLARING  0
#define oo_DESTROYING  1
#define oo_COPYING  0
#define oo_COMPARING  0
#define oo_VALIDATING_ENCODING  0
#define oo_READING  0
#define oo_READING_TEXT  0
#define oo_READING_BINARY  0
#define oo_WRITING  0
#define oo_WRITING_TEXT  0
#define oo_WRITING_BINARY  0
#define oo_DESCRIBING  0

/* End of file oo_DESTROY.h */
