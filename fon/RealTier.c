/* RealTier.c
 *
 * Copyright (C) 1992-2005 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * pb 2002/07/16 GPL
 * pb 2003/05/31 RealTier_formula
 * pb 2003/11/20 interpolate quadratically
 * pb 2005/03/02 RealTier_multiplyPart
 */

#include "RealTier.h"
#include "Formula.h"

#include "oo_DESTROY.h"
#include "RealTier_def.h"
#include "oo_COPY.h"
#include "RealTier_def.h"
#include "oo_EQUAL.h"
#include "RealTier_def.h"
#include "oo_WRITE_ASCII.h"
#include "RealTier_def.h"
#include "oo_READ_ASCII.h"
#include "RealTier_def.h"
#include "oo_WRITE_BINARY.h"
#include "RealTier_def.h"
#include "oo_READ_BINARY.h"
#include "RealTier_def.h"
#include "oo_DESCRIPTION.h"
#include "RealTier_def.h"

/********** class RealPoint **********/

class_methods (RealPoint, Data)
	class_method_local (RealPoint, destroy)
	class_method_local (RealPoint, copy)
	class_method_local (RealPoint, equal)
	class_method_local (RealPoint, writeAscii)
	class_method_local (RealPoint, readAscii)
	class_method_local (RealPoint, writeBinary)
	class_method_local (RealPoint, readBinary)
	class_method_local (RealPoint, description)
class_methods_end

RealPoint RealPoint_create (double time, double value) {
	RealPoint me = new (RealPoint);
	if (! me) return NULL;
	my time = time;
	my value = value;
	return me;
}

/********** class RealTier **********/

static double getNx (I) { iam (RealTier); return my points -> size; }
static double getX (I, long ix) { iam (RealTier); return ((RealPoint) my points -> item [ix]) -> time; }
static double getNcol (I) { iam (RealTier); return my points -> size; }
static double getVector (I, long icol) { iam (RealTier); return RealTier_getValueAtIndex (me, icol); }
static double getFunction1 (I, double x) { iam (RealTier); return RealTier_getValueAtTime (me, x); }

class_methods (RealTier, Function)
	class_method_local (RealTier, destroy)
	class_method_local (RealTier, copy)
	class_method_local (RealTier, equal)
	class_method_local (RealTier, writeAscii)
	class_method_local (RealTier, readAscii)
	class_method_local (RealTier, writeBinary)
	class_method_local (RealTier, readBinary)
	class_method_local (RealTier, description)
	class_method (getNx)
	class_method (getX)
	class_method (getNcol)
	class_method (getVector)
	class_method (getFunction1)
class_methods_end

int RealTier_init (I, double tmin, double tmax) {
	iam (RealTier);
	my xmin = tmin;
	my xmax = tmax;
	if (! (my points = SortedSetOfDouble_create ())) return 0;
	return 1;
}

RealTier RealTier_create (double tmin, double tmax) {
	RealTier me = new (RealTier);
	if (! me || ! RealTier_init (me, tmin, tmax)) { forget (me); return NULL; }
	return me;
}

int RealTier_addPoint (I, double t, double value) {
	iam (RealTier);
	RealPoint point = RealPoint_create (t, value);
	if (! point || ! Collection_addItem (my points, point)) return 0;
	return 1;
}

double RealTier_getValueAtIndex (I, long i) {
	iam (RealTier);
	if (i < 1 || i > my points -> size) return NUMundefined;
	return ((RealPoint) my points -> item [i]) -> value;
}

double RealTier_getValueAtTime (I, double t) {
	iam (RealTier);
	long n = my points -> size, ileft, iright;
	double tleft, tright, fleft, fright;
	RealPoint pointLeft, pointRight;
	if (n == 0) return NUMundefined;
	pointRight = my points -> item [1];
	if (t <= pointRight -> time) return pointRight -> value;   /* Constant extrapolation. */
	pointLeft = my points -> item [n];
	if (t >= pointLeft -> time) return pointLeft -> value;   /* Constant extrapolation. */
	Melder_assert (n >= 2);
	ileft = AnyTier_timeToLowIndex (me, t), iright = ileft + 1;
	Melder_assert (ileft >= 1 && iright <= n);
	pointLeft = my points -> item [ileft];
	pointRight = my points -> item [iright];
	tleft = pointLeft -> time, fleft = pointLeft -> value;
	tright = pointRight -> time, fright = pointRight -> value;
	return t == tright ? fright   /* Be very accurate. */
		: tleft == tright ? 0.5 * (fleft + fright)   /* Unusual, but possible; no preference. */
		: fleft + (t - tleft) * (fright - fleft) / (tright - tleft);   /* Linear interpolation. */
}

double RealTier_getMaximumValue (I) {
	iam (RealTier);
	double result = -1e300;
	long n = my points -> size, i;
	for (i = 1; i <= n; i ++) {
		RealPoint point = my points -> item [i];
		if (point -> value > result)
			result = point -> value;
	}
	return result;
}

double RealTier_getMinimumValue (I) {
	iam (RealTier);
	double result = +1e300;
	long n = my points -> size, i;
	for (i = 1; i <= n; i ++) {
		RealPoint point = my points -> item [i];
		if (point -> value < result)
			result = point -> value;
	}
	return result;
}

double RealTier_getArea (I, double tmin, double tmax) {
	iam (RealTier);
	long n = my points -> size, imin, imax, i;
	RealPoint *points = (RealPoint *) my points -> item;
	double area = 0.0;
	if (n == 0) return NUMundefined;
	if (n == 1) return (tmax - tmin) * points [1] -> value;
	imin = AnyTier_timeToLowIndex (me, tmin);
	if (imin == n) return (tmax - tmin) * points [n] -> value;
	imax = AnyTier_timeToHighIndex (me, tmax);
	if (imax == 1) return (tmax - tmin) * points [1] -> value;
	Melder_assert (imin < n);
	Melder_assert (imax > 1);
	/*
	 * Add the areas between the points.
	 * This works even if imin is 0 (offleft) and/or imax is n + 1 (offright).
	 */
	for (i = imin; i < imax; i ++) {
		double tleft, fleft, tright, fright;
		if (i == imin) tleft = tmin, fleft = RealTier_getValueAtTime (me, tmin);
		else tleft = points [i] -> time, fleft = points [i] -> value;
		if (i + 1 == imax) tright = tmax, fright = RealTier_getValueAtTime (me, tmax);
		else tright = points [i + 1] -> time, fright = points [i + 1] -> value;
		area += 0.5 * (fleft + fright) * (tright - tleft);
	}
	return area;
}

double RealTier_getMean_curve (I, double tmin, double tmax) {
	iam (RealTier);
	double area;
	if (tmax <= tmin) { tmin = my xmin; tmax = my xmax; }   /* Autowindow. */
	area = RealTier_getArea (me, tmin, tmax);
	if (area == NUMundefined) return NUMundefined;
	return area / (tmax - tmin);
}

double RealTier_getStandardDeviation_curve (I, double tmin, double tmax) {
	iam (RealTier);
	long n = my points -> size, imin, imax, i;
	RealPoint *points = (RealPoint *) my points -> item;
	double mean, integral = 0.0;
	if (tmax <= tmin) { tmin = my xmin; tmax = my xmax; }   /* Autowindow. */
	if (n == 0) return NUMundefined;
	if (n == 1) return 0.0;
	imin = AnyTier_timeToLowIndex (me, tmin);
	if (imin == n) return 0.0;
	imax = AnyTier_timeToHighIndex (me, tmax);
	if (imax == 1) return 0.0;
	Melder_assert (imin < n);
	Melder_assert (imax > 1);
	/*
	 * Add the areas between the points.
	 * This works even if imin is 0 (offleft) and/or imax is n + 1 (offright).
	 */
	mean = RealTier_getMean_curve (me, tmin, tmax);
	for (i = imin; i < imax; i ++) {
		double tleft, fleft, tright, fright, sum, diff;
		if (i == imin) tleft = tmin, fleft = RealTier_getValueAtTime (me, tmin);
		else tleft = points [i] -> time, fleft = points [i] -> value - mean;
		if (i + 1 == imax) tright = tmax, fright = RealTier_getValueAtTime (me, tmax);
		else tright = points [i + 1] -> time, fright = points [i + 1] -> value - mean;
		/*
		 * The area is integral dt f^2
		 *   = integral dt [f1 + (f2-f1)/(t2-t1) (t-t1)]^2
		 *   = int dt f1^2 + int dt 2 f1 (f2-f1)/(t2-t1) (t-t1) + int dt [(f2-f1)/(t2-t1)]^2 (t-t1)^2
		 *   = f1^2 (t2-t1) + f1 (f2-f1)/(t2-t1) (t2-t1)^2 + 1/3 [(f2-f1)/(t2-t1)]^2 (t2-t1)^3
		 *   = (t2-t1) [f1 f2 + 1/3 (f2-f1)^2]
		 *   = (t2-t1) (f1^2 + f2^2 + 1/3 f1 f2)
		 *   = (t2-t1) [1/4 (f1+f2)^2 + 1/12 (f1-f2)^2]
		 * In the last expression, we have a sum of squares, which is computationally best.
		 */
		sum = fleft + fright;
		diff = fleft - fright;
		integral += (sum * sum + (1.0/3.0) * diff * diff) * (tright - tleft);
	}
	return sqrt (0.25 * integral / (tmax - tmin));
}

double RealTier_getMean_points (I, double tmin, double tmax) {
	iam (RealTier);
	long n = my points -> size, imin, imax, i;
	double sum = 0.0;
	RealPoint *points = (RealPoint *) my points -> item;
	if (tmax <= tmin) { tmin = my xmin; tmax = my xmax; }   /* Autowindow. */
	n = AnyTier_getWindowPoints (me, tmin, tmax, & imin, & imax);
	if (n == 0) return NUMundefined;
	for (i = imin; i <= imax; i ++)
		sum += points [i] -> value;
	return sum / n;
}

double RealTier_getStandardDeviation_points (I, double tmin, double tmax) {
	iam (RealTier);
	long n = my points -> size, imin, imax, i;
	double mean, sum = 0.0;
	RealPoint *points = (RealPoint *) my points -> item;
	if (tmax <= tmin) { tmin = my xmin; tmax = my xmax; }   /* Autowindow. */
	n = AnyTier_getWindowPoints (me, tmin, tmax, & imin, & imax);
	if (n < 2) return NUMundefined;
	mean = RealTier_getMean_points (me, tmin, tmax);
	for (i = imin; i <= imax; i ++) {
		double diff = points [i] -> value - mean;
		sum += diff * diff;
	}
	return sqrt (sum / (n - 1));
}

void RealTier_multiplyPart (I, double tmin, double tmax, double factor) {
	iam (RealTier);
	long ipoint;
	for (ipoint = 1; ipoint <= my points -> size; ipoint ++) {
		RealPoint point = my points -> item [ipoint];
		double t = point -> time;
		if (t >= tmin && t <= tmax) {
			point -> value *= factor;
		}
	}
}

void RealTier_draw (I, Graphics g, double tmin, double tmax, double fmin, double fmax,
	int garnish, const char *quantity)
{
	iam (RealTier);
	long n = my points -> size, imin, imax, i;
	if (tmax <= tmin) { tmin = my xmin; tmax = my xmax; }
	Graphics_setWindow (g, tmin, tmax, fmin, fmax);
	Graphics_setInner (g);
	imin = AnyTier_timeToHighIndex (me, tmin);
	imax = AnyTier_timeToLowIndex (me, tmax);
	if (n == 0) {
	} else if (imax < imin) {
		double fleft = RealTier_getValueAtTime (me, tmin);
		double fright = RealTier_getValueAtTime (me, tmax);
		Graphics_line (g, tmin, fleft, tmax, fright);
	} else for (i = imin; i <= imax; i ++) {
		RealPoint point = my points -> item [i];
		double t = point -> time, f = point -> value;
		Graphics_fillCircle_mm (g, t, f, 1);
		if (i == 1)
			Graphics_line (g, tmin, f, t, f);
		else if (i == imin)
			Graphics_line (g, t, f, tmin, RealTier_getValueAtTime (me, tmin));
		if (i == n)
			Graphics_line (g, t, f, tmax, f);
		else if (i == imax)
			Graphics_line (g, t, f, tmax, RealTier_getValueAtTime (me, tmax));
		else {
			RealPoint pointRight = my points -> item [i + 1];
			Graphics_line (g, t, f, pointRight -> time, pointRight -> value);
		}
	}
	Graphics_unsetInner (g);
	if (garnish) {
		Graphics_drawInnerBox (g);
		Graphics_textBottom (g, TRUE, "Time (s)");
		Graphics_marksBottom (g, 2, TRUE, TRUE, FALSE);
		Graphics_marksLeft (g, 2, TRUE, TRUE, FALSE);
		if (quantity) Graphics_textLeft (g, TRUE, quantity);
	}
}

TableOfReal RealTier_downto_TableOfReal (I, const char *timeLabel, const char *valueLabel) {
	iam (RealTier);
	TableOfReal thee = NULL;
	long i;

	thee = TableOfReal_create (my points -> size, 2); cherror
	TableOfReal_setColumnLabel (thee, 1, timeLabel);
	TableOfReal_setColumnLabel (thee, 2, valueLabel);
	for (i = 1; i <= my points -> size; i ++) {
		RealPoint point = my points -> item [i];
		thy data [i] [1] = point -> time;
		thy data [i] [2] = point -> value;
	}
end:
	iferror forget (thee);
	return thee;
}

int RealTier_interpolateQuadratically (I, long numberOfPointsPerParabola, int logarithmically) {
	iam (RealTier);
	long ipoint, inewpoint;
	RealTier thee = Data_copy (me);
	for (ipoint = 1; ipoint < my points -> size; ipoint ++) {
		RealPoint point1 = my points -> item [ipoint], point2 = my points -> item [ipoint + 1];
		double time1 = point1 -> time, time2 = point2 -> time, tmid = 0.5 * (time1 + time2);
		double value1 = point1 -> value, value2 = point2 -> value, valuemid;
		double timeStep = (tmid - time1) / (numberOfPointsPerParabola + 1);
		if (logarithmically) value1 = log (value1), value2 = log (value2);
		valuemid = 0.5 * (value1 + value2);
		/*
		 * Left from the midpoint.
		 */
		for (inewpoint = 1; inewpoint <= numberOfPointsPerParabola; inewpoint ++) {
			double newTime = time1 + inewpoint * timeStep;
			double phase = (newTime - time1) / (tmid - time1);
			double newValue = value1 + (valuemid - value1) * phase * phase;
			if (logarithmically) newValue = exp (newValue);
			RealTier_addPoint (thee, newTime, newValue); cherror
		}
		/*
		 * The midpoint.
		 */
		RealTier_addPoint (thee, tmid, logarithmically ? exp (valuemid) : valuemid); cherror
		/*
		 * Right from the midpoint.
		 */
		for (inewpoint = 1; inewpoint <= numberOfPointsPerParabola; inewpoint ++) {
			double newTime = tmid + inewpoint * timeStep;
			double phase = (time2 - newTime) / (time2 - tmid);
			double newValue = value2 + (valuemid - value2) * phase * phase;
			if (logarithmically) newValue = exp (newValue);
			RealTier_addPoint (thee, newTime, newValue); cherror
		}
	}
end:
	iferror { forget (thee); return 0; }
	Thing_swap (me, thee);
	forget (thee);
	return 1;
}

RealTier Vector_to_RealTier (I) {
	iam (Vector);
	RealTier thee = RealTier_create (my xmin, my xmax);
	long i;
	if (! thee) return NULL;
	for (i = 1; i <= my nx; i ++) {
		if (! RealTier_addPoint (thee, Sampled_indexToX (me, i), my z [1] [i]))
			{ forget (thee); return NULL; }
	}
	return thee;
}

RealTier Vector_to_RealTier_peaks (I) {
	iam (Vector);
	RealTier thee = RealTier_create (my xmin, my xmax);
	long i;
	if (! thee) return NULL;
	for (i = 2; i < my nx; i ++) {
		double left = my z [1] [i - 1], centre = my z [1] [i], right = my z [1] [i + 1];
		if (left <= centre && right < centre) {
			double x, maximum;
			Vector_getMaximumAndX (me, my x1 + (i - 2.5) * my dx,
				my x1 + (i + 0.5) * my dx, NUM_PEAK_INTERPOLATE_PARABOLIC,
				& maximum, & x);
			if (! RealTier_addPoint (thee, x, maximum))
				{ forget (thee); return NULL; }
		}
	}
	return thee;
}

RealTier Vector_to_RealTier_valleys (I) {
	iam (Vector);
	RealTier thee = RealTier_create (my xmin, my xmax);
	long i;
	if (! thee) return NULL;
	for (i = 2; i < my nx; i ++) {
		double left = my z [1] [i - 1], centre = my z [1] [i], right = my z [1] [i + 1];
		if (left >= centre && right > centre) {
			double x, minimum;
			Vector_getMinimumAndX (me, my x1 + (i - 2.5) * my dx,
				my x1 + (i + 0.5) * my dx, NUM_PEAK_INTERPOLATE_PARABOLIC,
				& minimum, & x);
			if (! RealTier_addPoint (thee, x, minimum))
				{ forget (thee); return NULL; }
		}
	}
	return thee;
}

RealTier PointProcess_upto_RealTier (PointProcess me, double value) {
	long i;
	RealTier thee = RealTier_create (my xmin, my xmax);
	if (! thee) return NULL;
	for (i = 1; i <= my nt; i ++)
		if (! RealTier_addPoint (thee, my t [i], value)) { forget (thee); return NULL; }
	return thee;
}

int RealTier_formula (I, const char *expression, thou) {
	iam (RealTier);
	thouart (RealTier);
	long icol;
	if (! Formula_compile (NULL, me, expression, FALSE, TRUE)) return 0;
	if (thee == NULL) thee = me;
	for (icol = 1; icol <= my points -> size; icol ++) {
		double result;
		if (! Formula_run (0, icol, & result, NULL)) return 0;
		((RealPoint) thy points -> item [icol]) -> value = result;
	}
	return 1;
}

/* End of file RealTier.c */