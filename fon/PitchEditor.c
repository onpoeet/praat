/* PitchEditor.c
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
 * pb 2004/04/13 less flashing
 * pb 2004/05/10 undefined pitch is NUMundefined rather than 0.0
 * pb 2004/10/16 struct PitchCandidate -> struct structPitchCandidate
 * pb 2005/06/16 units
 */

#include "Pitch_to_Sound.h"
#include "PitchEditor.h"
#include "EditorM.h"

#define HEIGHT_UNV  3.0
#define HEIGHT_INTENS  6.0
#define RADIUS  2.5

#define PitchEditor_members FunctionEditor_members
#define PitchEditor_methods FunctionEditor_methods
class_create_opaque (PitchEditor, FunctionEditor)

/********** MENU COMMANDS **********/

FORM (PitchEditor, cb_setCeiling, "Change ceiling", 0)
	POSITIVE ("Ceiling (Hertz)", "600")
	OK
Pitch pitch = my data;
SET_REAL ("Ceiling", pitch -> ceiling)
DO
	Pitch pitch = my data;
	Editor_save (me, "Change ceiling");
	Pitch_setCeiling (pitch, GET_REAL ("Ceiling"));
	FunctionEditor_redraw (me);
	Editor_broadcastChange (me);
END

FORM (PitchEditor, cb_pathFinder, "Path finder", 0);
	REAL ("Silence threshold", "0.03")
	REAL ("Voicing threshold", "0.45")
	REAL ("Octave cost", "0.01")
	REAL ("Octave-jump cost", "0.35")
	REAL ("Voiced/unvoiced cost", "0.14")
	POSITIVE ("Ceiling (Hertz)", "600")
	BOOLEAN ("Pull formants", 0)
	OK
Pitch pitch = my data;
SET_REAL ("Ceiling", pitch -> ceiling)
DO
	Pitch pitch = my data;
	Editor_save (me, "Path finder");
	Pitch_pathFinder (pitch,
		GET_REAL ("Silence threshold"), GET_REAL ("Voicing threshold"),
		GET_REAL ("Octave cost"), GET_REAL ("Octave-jump cost"),
		GET_REAL ("Voiced/unvoiced cost"), GET_REAL ("Ceiling"), GET_INTEGER ("Pull formants"));
	FunctionEditor_redraw (me);
	Editor_broadcastChange (me);
END

DIRECT (PitchEditor, cb_getPitch)
	if (my startSelection == my endSelection) {
		Melder_informationReal (Pitch_getValueAtTime (my data, my startSelection, Pitch_UNIT_HERTZ, 1), "Hertz");
	} else {
		Melder_informationReal (Pitch_getMean (my data, my startSelection, my endSelection, Pitch_UNIT_HERTZ), "Hertz");
	}
END

DIRECT (PitchEditor, cb_octaveUp)
	Pitch pitch = my data;
	Editor_save (me, "Octave up");
	Pitch_step (pitch, 2.0, 0.1, my startSelection, my endSelection);
	FunctionEditor_redraw (me);
	Editor_broadcastChange (me);
END

DIRECT (PitchEditor, cb_fifthUp)
	Pitch pitch = my data;
	Editor_save (me, "Fifth up");
	Pitch_step (pitch, 1.5, 0.1, my startSelection, my endSelection);
	FunctionEditor_redraw (me);
	Editor_broadcastChange (me);
END

DIRECT (PitchEditor, cb_fifthDown)
	Pitch pitch = my data;
	Editor_save (me, "Fifth down");
	Pitch_step (pitch, 1 / 1.5, 0.1, my startSelection, my endSelection);
	FunctionEditor_redraw (me);
	Editor_broadcastChange (me);
END

DIRECT (PitchEditor, cb_octaveDown)
	Pitch pitch = my data;
	Editor_save (me, "Octave down");
	Pitch_step (pitch, 0.5, 0.1, my startSelection, my endSelection);
	FunctionEditor_redraw (me);
	Editor_broadcastChange (me);
END

DIRECT (PitchEditor, cb_voiceless)
	Pitch pitch = my data;
	long ileft = Sampled_xToHighIndex (pitch, my startSelection), i, cand;
	long iright = Sampled_xToLowIndex (pitch, my endSelection);
	if (ileft < 1) ileft = 1;
	if (iright > pitch -> nx) iright = pitch -> nx;
	Editor_save (me, "Unvoice");
	for (i = ileft; i <= iright; i ++) {
		Pitch_Frame frame = & pitch -> frame [i];
		for (cand = 1; cand <= frame -> nCandidates; cand ++) {
			if (frame -> candidate [cand]. frequency == 0.0) {
				struct structPitch_Candidate help = frame -> candidate [1];
				frame -> candidate [1] = frame -> candidate [cand];
				frame -> candidate [cand] = help;
			}
		}
	}
	FunctionEditor_redraw (me);
	Editor_broadcastChange (me);
END

DIRECT (PitchEditor, cb_PitchEditorHelp) Melder_help ("PitchEditor"); END
DIRECT (PitchEditor, cb_PitchHelp) Melder_help ("Pitch"); END

static void createMenus (I) {
	iam (PitchEditor);
	inherited (PitchEditor) createMenus (me);

	Editor_addCommand (me, "Edit", "Change ceiling...", 0, cb_setCeiling);
	Editor_addCommand (me, "Edit", "Path finder...", 0, cb_pathFinder);

	Editor_addCommand (me, "Query", "-- pitch --", 0, NULL);
	Editor_addCommand (me, "Query", "Get pitch", motif_F10, cb_getPitch);

	Editor_addMenu (me, "Selection", 0);
	Editor_addCommand (me, "Selection", "Unvoice", 0, cb_voiceless);
	Editor_addCommand (me, "Selection", "-- up and down --", 0, NULL);
	Editor_addCommand (me, "Selection", "Octave up", 0, cb_octaveUp);
	Editor_addCommand (me, "Selection", "Fifth up", 0, cb_fifthUp);
	Editor_addCommand (me, "Selection", "Fifth down", 0, cb_fifthDown);
	Editor_addCommand (me, "Selection", "Octave down", 0, cb_octaveDown);

	Editor_addCommand (me, "Help", "PitchEditor help", '?', cb_PitchEditorHelp);
	Editor_addCommand (me, "Help", "Pitch help", 0, cb_PitchHelp);
}
	
/********** DRAWING AREA **********/

static void draw (I) {
	iam (PitchEditor);
	Pitch pitch = my data;
	long it, it1, it2;
	double dyUnv, dyIntens;

	Graphics_setWindow (my graphics, 0, 1, 0, 1);
	Graphics_setColour (my graphics, Graphics_WHITE);
	Graphics_fillRectangle (my graphics, 0, 1, 0, 1);
	Graphics_setColour (my graphics, Graphics_BLACK);
	Graphics_rectangle (my graphics, 0, 1, 0, 1);

	dyUnv = Graphics_dyMMtoWC (my graphics, HEIGHT_UNV);
	dyIntens = Graphics_dyMMtoWC (my graphics, HEIGHT_INTENS);

	Sampled_getWindowSamples (pitch, my startWindow, my endWindow, & it1, & it2);

	/*
	 * Show pitch.
	 */
	{
		long f, df =
			pitch -> ceiling > 10000 ? 2000 :
			pitch -> ceiling > 5000 ? 1000 :
			pitch -> ceiling > 2000 ? 500 :
			pitch -> ceiling > 800 ? 200 :
			pitch -> ceiling > 400 ? 100 :
			50;
		double radius;
		Graphics_Viewport previous;
		previous = Graphics_insetViewport (my graphics, 0, 1, dyUnv, 1 - dyIntens);
		Graphics_setWindow (my graphics, my startWindow, my endWindow, 0, pitch -> ceiling);
		radius = Graphics_dxMMtoWC (my graphics, RADIUS);

		/* Horizontal hair at current pitch. */

		if (my startSelection == my endSelection && my startSelection >= my startWindow && my startSelection <= my endWindow) {
			double f = Pitch_getValueAtTime (pitch, my startSelection, Pitch_UNIT_HERTZ, Pitch_LINEAR);
			if (NUMdefined (f)) {
				Graphics_setColour (my graphics, Graphics_RED);
				Graphics_line (my graphics, my startWindow - radius, f, my endWindow, f);
				Graphics_setTextAlignment (my graphics, Graphics_RIGHT, Graphics_HALF);
				Graphics_printf (my graphics, my startWindow - radius, f, "%.2f", f);
			}
		}

		/* Horizontal scaling lines. */

		Graphics_setColour (my graphics, Graphics_BLUE);
		Graphics_setLineType (my graphics, Graphics_DOTTED);
		Graphics_setTextAlignment (my graphics, Graphics_LEFT, Graphics_HALF);
		for (f = df; f <= pitch -> ceiling; f += df) {
			Graphics_line (my graphics, my startWindow, f, my endWindow, f);
			Graphics_printf (my graphics, my endWindow + radius/2, f, "%ld Hz", f);
		}
		Graphics_setLineType (my graphics, Graphics_DRAWN);

		/* Show candidates. */

		for (it = it1; it <= it2; it ++) {
			Pitch_Frame frame = & pitch -> frame [it];
			double t = Sampled_indexToX (pitch, it);
			int icand;
			float f = frame -> candidate [1]. frequency;
			if (f > 0.0 && f < pitch -> ceiling) {
				Graphics_setColour (my graphics, Graphics_MAGENTA);
				Graphics_fillCircle_mm (my graphics, t, f, RADIUS * 2);
			}
			Graphics_setColour (my graphics, Graphics_BLACK);
			Graphics_setTextAlignment (my graphics, Graphics_CENTRE, Graphics_HALF);
			for (icand = 1; icand <= frame -> nCandidates; icand ++) {
				int strength = floor (10 * frame -> candidate [icand]. strength + 0.5);
				f = frame -> candidate [icand]. frequency;
				if (strength > 9) strength = 9;
				if (f > 0 && f <= pitch -> ceiling) Graphics_printf (my graphics, t, f, "%d", strength);
			}
		}
		Graphics_resetViewport (my graphics, previous);
	}

	/*
	 * Show intensity.
	 */
	{
		Graphics_Viewport previous;
		previous = Graphics_insetViewport (my graphics, 0, 1, 1 - dyIntens, 1);
		Graphics_setWindow (my graphics, my startWindow, my endWindow, 0, 1);
		Graphics_setColour (my graphics, Graphics_BLACK);
		Graphics_setTextAlignment (my graphics, Graphics_RIGHT, Graphics_HALF);
		Graphics_text (my graphics, my startWindow, 0.5, "intens");
		Graphics_setTextAlignment (my graphics, Graphics_LEFT, Graphics_HALF);
		Graphics_text (my graphics, my endWindow, 0.5, "intens");
		Graphics_setTextAlignment (my graphics, Graphics_CENTRE, Graphics_HALF);
		for (it = it1; it <= it2; it ++) {
			Pitch_Frame frame = & pitch -> frame [it];
			double t = Sampled_indexToX (pitch, it);
			int strength = floor (10 * frame -> intensity + 0.5);   /* Map 0.0-1.0 to 0-9 */
			if (strength > 9) strength = 9;
			Graphics_printf (my graphics, t, 0.5, "%d", strength);
		}
		Graphics_resetViewport (my graphics, previous);
	}

	if (it1 > 1) it1 -= 1;
	if (it2 < pitch -> nx) it2 += 1;

	/*
	 * Show voicelessness.
	 */
	{
		Graphics_Viewport previous;
		previous = Graphics_insetViewport (my graphics, 0, 1, 0, dyUnv);
		Graphics_setColour (my graphics, Graphics_BLUE);
		Graphics_line (my graphics, my startWindow, 1, my endWindow, 1);
		Graphics_setTextAlignment (my graphics, Graphics_RIGHT, Graphics_HALF);
		Graphics_text (my graphics, my startWindow, 0.5, "Unv");
		Graphics_setTextAlignment (my graphics, Graphics_LEFT, Graphics_HALF);
		Graphics_text (my graphics, my endWindow, 0.5, "Unv");
		for (it = it1; it <= it2; it ++) {
			Pitch_Frame frame = & pitch -> frame [it];
			double t = Sampled_indexToX (pitch, it), tleft = t - 0.5 * pitch -> dx, tright = t + 0.5 * pitch -> dx;
			float f = frame -> candidate [1]. frequency;
			if (f != 0.0 || tright <= my startWindow || tleft >= my endWindow) continue;
			if (tleft < my startWindow) tleft = my startWindow;
			if (tright > my endWindow) tright = my endWindow;
			Graphics_fillRectangle (my graphics, tleft, tright, 0, 1);
		}
		Graphics_setColour (my graphics, Graphics_BLACK);
		Graphics_resetViewport (my graphics, previous);
	}
}

static void play (I, double tmin, double tmax) {
	iam (PitchEditor);
	if (! Pitch_hum (my data, tmin, tmax)) Melder_flushError (NULL);
}

static int click (I, double xWC, double yWC, int dummy) {
	iam (PitchEditor);
	Pitch pitch = my data;
	double dyUnv = Graphics_dyMMtoWC (my graphics, HEIGHT_UNV);
	double dyIntens = Graphics_dyMMtoWC (my graphics, HEIGHT_INTENS);
	double frequency = (yWC - dyUnv) / (1 - dyIntens - dyUnv) * pitch -> ceiling, tmid;
	double minimumDf = 1e30;
	int cand, bestCandidate = -1;

	long ibestFrame;
	Pitch_Frame bestFrame;
	ibestFrame = Sampled_xToNearestIndex (pitch, xWC);
	if (ibestFrame < 1) ibestFrame = 1;
	if (ibestFrame > pitch -> nx) ibestFrame = pitch -> nx;
	bestFrame = & pitch -> frame [ibestFrame];

	tmid = Sampled_indexToX (pitch, ibestFrame);
	for (cand = 1; cand <= bestFrame -> nCandidates; cand ++) {
		double df = frequency - bestFrame -> candidate [cand]. frequency;
		if (fabs (df) < minimumDf) {
			minimumDf = fabs (df);
			bestCandidate = cand;
		}
	}
	if (bestCandidate != -1) {
		double bestFrequency = bestFrame -> candidate [bestCandidate]. frequency;
		double distanceWC = (frequency - bestFrequency) / pitch -> ceiling * (1 - dyIntens - dyUnv);
		double dx_mm = Graphics_dxWCtoMM (my graphics, xWC - tmid), dy_mm = Graphics_dyWCtoMM (my graphics, distanceWC);
		if (bestFrequency < pitch -> ceiling &&   /* Above ceiling: ignore. */
		    (bestFrequency <= 0.0 && fabs (xWC - tmid) <= 0.5 * pitch -> dx && frequency <= 0.0 ||   /* Voiceless: click within frame. */
		     bestFrequency > 0.0 && dx_mm * dx_mm + dy_mm * dy_mm <= RADIUS * RADIUS))   /* Voiced: click within circle. */
		{
			struct structPitch_Candidate help = bestFrame -> candidate [1];
			Editor_save (me, "Change path");
			bestFrame -> candidate [1] = bestFrame -> candidate [bestCandidate];
			bestFrame -> candidate [bestCandidate] = help;
			FunctionEditor_redraw (me);
			Editor_broadcastChange (me);
			my startSelection = my endSelection = tmid;   /* Cursor will snap to candidate. */
			return 1;
		} else {
			return inherited (PitchEditor) click (me, xWC, yWC, dummy);   /* Move cursor or drag selection. */
		}
	}
	return inherited (PitchEditor) click (me, xWC, yWC, dummy);   /* Move cursor or drag selection. */
}

class_methods (PitchEditor, FunctionEditor)
	class_method (createMenus)
	class_method (draw)
	class_method (play)
	class_method (click)
class_methods_end

PitchEditor PitchEditor_create (Widget parent, const char *title, Pitch pitch) {
	PitchEditor me = new (PitchEditor);
	if (! me || ! FunctionEditor_init (me, parent, title, pitch))
		return NULL;
	return me;
}

/* End of file PitchEditor.c */