/*
Copyright (C) 1999-2007 id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "stdafx.h"
//#include "qe3.h"

/*

  drag either multiple brushes, or select plane points from
  a single brush.

*/

extern int g_nPatchClickedView;

qboolean	drag_ok;
vec3_t	drag_xvec;
vec3_t	drag_yvec;

//static	int	buttonstate;
int	pressx, pressy;
static	vec3_t pressdelta;
static	vec3_t vPressStart;
//static	int	buttonx, buttony;


//int		num_move_points;
//float	*move_points[1024];

int		lastx, lasty;

qboolean	drag_first;


void	AxializeVector (vec3_t v)
{
	vec3_t	a;
	float	o;
	int		i;

	if (!v[0] && !v[1])
		return;
	if (!v[1] && !v[2])
		return;
	if (!v[0] && !v[2])
		return;

	for (i=0 ; i<3 ; i++)
		a[i] = fabs(v[i]);
	if (a[0] > a[1] && a[0] > a[2])
		i = 0;
	else if (a[1] > a[0] && a[1] > a[2])
		i = 1;
	else
		i = 2;

	o = v[i];
	VectorCopy (vec3_origin, v);
	if (o<0)
		v[i] = -1;
	else
		v[i] = 1;

}

/*
===========
Drag_Setup
===========
*/
extern void SelectCurvePointByRay (vec3_t org, vec3_t dir, int buttons);

void Drag_Setup (int x, int y, int buttons,
		 vec3_t xaxis, vec3_t yaxis,
		 vec3_t origin, vec3_t dir)
{
  trace_t	t;
  face_t	*f;

  drag_first = true;

  VectorCopy (vec3_origin, pressdelta);
  pressx = x;
  pressy = y;

	// snap to nearest axis for camwindow drags
  VectorCopy (xaxis, drag_xvec);
  AxializeVector (drag_xvec);
  VectorCopy (yaxis, drag_yvec);
  AxializeVector (drag_yvec);

  if (g_qeglobals.d_select_mode == sel_curvepoint)
  {
    SelectCurvePointByRay (origin, dir, buttons);

    if(g_qeglobals.d_select_mode == sel_area)
    {
      drag_ok = true;

			if(g_nPatchClickedView == W_CAMERA ) {
				VectorSet( g_qeglobals.d_vAreaTL, x, y, 0 );
				VectorSet( g_qeglobals.d_vAreaBR, x, y, 0 );
			}
    }
    else if (g_qeglobals.d_num_move_points) // don't add an undo if there are no points selected
    {
      drag_ok = true;
      Sys_UpdateWindows(W_ALL);
      Undo_Start("drag curve point");
      Undo_AddBrushList(&selected_brushes);
    }
    return;
  }
  else
  {
    g_qeglobals.d_num_move_points = 0;
  }

  if (g_qeglobals.d_select_mode == sel_areatall || g_qeglobals.d_select_mode == sel_brush_area_partial_tall)
  {
    VectorCopy(origin, g_qeglobals.d_vAreaTL);
    VectorCopy(origin, g_qeglobals.d_vAreaBR);

    Sys_UpdateWindows(W_ALL);

    drag_ok = true;
    return;
  }

  if (selected_brushes.next == &selected_brushes)
  {
    //in this case a new brush is created when the dragging
    //takes place in the XYWnd, An useless undo is created
    //when the dragging takes place in the CamWnd
    Undo_Start("create brush");

    Sys_Status("No selection to drag", 0);
    return;
  }

  if (g_qeglobals.d_select_mode == sel_vertex)
  {
    SelectVertexByRay (origin, dir);
    if (g_qeglobals.d_num_move_points)
    {
      drag_ok = true;
      Undo_Start("drag vertex");
      Undo_AddBrushList(&selected_brushes);
      // Need an update here for highlighting selected vertices
      Sys_UpdateWindows(W_XY | W_CAMERA);
      return;
    }
  }

  if (g_qeglobals.d_select_mode == sel_edge)
  {
    SelectEdgeByRay (origin, dir);
    if (g_qeglobals.d_num_move_points)
    {
      drag_ok = true;
      Undo_Start("drag edge");
      Undo_AddBrushList(&selected_brushes);
      return;
    }
  }

  //
  // check for direct hit first
  //
  t = Test_Ray (origin, dir, true);
  if (t.selected)
  {
    drag_ok = true;

    Undo_Start("drag selection");
    Undo_AddBrushList(&selected_brushes);

    if (buttons == (MK_LBUTTON|MK_CONTROL) )
    {
      Sys_Printf ("Shear dragging face\n");
      Brush_SelectFaceForDragging (t.brush, t.face, true);
    }
    else if (buttons == (MK_LBUTTON|MK_CONTROL|MK_SHIFT) )
    {
      Sys_Printf ("Sticky dragging brush\n");
      for (f=t.brush->brush_faces ; f ; f=f->next)
    	Brush_SelectFaceForDragging (t.brush, f, false);
    }
    else
      Sys_Printf ("Dragging entire selection\n");

    return;
  }

  if (g_qeglobals.d_select_mode == sel_vertex || g_qeglobals.d_select_mode == sel_edge)
    return;

  //
  // check for side hit
  //
  // multiple brushes selected?
  if (selected_brushes.next->next != &selected_brushes)
  {
    // yes, special handling
    bool bOK = (g_PrefsDlg.m_bALTEdge) ? Sys_AltDown() : true;
    if (bOK)
    {
      for (brush_t* pBrush = selected_brushes.next ; pBrush != &selected_brushes ; pBrush = pBrush->next)
      {
      	if (buttons & MK_CONTROL)
      	  Brush_SideSelect (pBrush, origin, dir, true);
      	else
      	  Brush_SideSelect (pBrush, origin, dir, false);
      }
    }
    else
    {
      Sys_Printf ("press ALT to drag multiple edges\n");
      return;
    }
  }
  else
  {
    // single select.. trying to drag fixed entities handle themselves and just move
    if (buttons & MK_CONTROL)
      Brush_SideSelect (selected_brushes.next, origin, dir, true);
    else
      Brush_SideSelect (selected_brushes.next, origin, dir, false);
  }

  Sys_Printf ("Side stretch\n");
  drag_ok = true;

  Undo_Start("side stretch");
  Undo_AddBrushList(&selected_brushes);
}

entity_t *peLink;

void UpdateTarget(vec3_t origin, vec3_t dir)
{
	trace_t	t;
	entity_t *pe;
	int i;
	char sz[128];

	t = Test_Ray (origin, dir, 0);

	if (!t.brush)
		return;

	pe = t.brush->owner;

	if (pe == NULL)
		return;

	// is this the first?
	if (peLink != NULL)
	{

		// Get the target id from out current target
		// if there is no id, make one

		i = IntForKey(pe, "target");
		if (i <= 0)
		{
			i = GetUniqueTargetId(1);
			sprintf(sz, "%d", i);

			SetKeyValue(pe, "target", sz);
		}

		// set the target # into our src

		sprintf(sz, "%d", i);
		SetKeyValue(peLink, "targetname", sz);

		Sys_UpdateWindows(W_ENTITY);

	}

	// promote the target to the src

	peLink = pe;

}

/*
===========
Drag_Begin
//++timo test three button mouse and three button emulation here ?
===========
*/

// mfn TODO: this is really ugly: the shift-LBUTTON (brush selection) code had
// to be moved into Drag_MouseUp() so we can detect shift-LBUTTON drags (with
// modifiers SHIFT or ALT) to switch to area selection mode. However the
// necessary information for the shift-LBUTTON brush selection isn't available
// there, in case it was no drag. Thus we store all required information in
// these variables, which are then only used in Drag_MouseUp() in this specific
// case.
static bool drag_done; // state whether there was a drag attempt or not. it's false and only gets set to true in Drag_MouseMoved() and to be checked in Drag_MouseUp() later
static bool l_sf_camera; // Variable to transport value from Drag_Begin() to Drag_MouseUp()
static vec3_t l_origin, l_dir; // Variables to transport value from Drag_Begin() to Drag_MouseUp()

void Drag_Begin (int x, int y, int buttons,
		   vec3_t xaxis, vec3_t yaxis,
		   vec3_t origin, vec3_t dir, bool sf_camera)
{
	trace_t	t;
  bool altdown;
  int nFlag;

	drag_ok = false;
	VectorCopy (vec3_origin, pressdelta);
	VectorCopy (vec3_origin, vPressStart);

	drag_first = true;
	peLink = NULL;

  altdown = Sys_AltDown();

  // store for eventual later use in Drag_MouseUp()
  l_sf_camera = sf_camera;
  VectorCopy (origin, l_origin);
  VectorCopy (dir, l_dir);
  // indicate there was no drag yet, we're still in Drag_Begin() here
  drag_done = false;

  // (shift-)alt-LBUTTON = area select completely tall
  if ( !sf_camera &&
      ( g_PrefsDlg.m_bALTEdge ? buttons == (MK_LBUTTON | MK_CONTROL | MK_SHIFT) : (buttons == MK_LBUTTON || buttons == (MK_LBUTTON | MK_CONTROL | MK_SHIFT)) ) &&
      altdown && g_qeglobals.d_select_mode != sel_curvepoint)
  {
    if (g_pParentWnd->ActiveXY()->AreaSelectOK())
    {
      g_qeglobals.d_select_mode = sel_areatall;

      Drag_Setup (x, y, buttons, xaxis, yaxis, origin, dir);
      return;
    }
  }

  // shift-LBUTTON = area select partial tall
  if ( !sf_camera && buttons == (MK_LBUTTON | MK_SHIFT) && g_qeglobals.d_select_mode != sel_curvepoint) 
  {
    if (g_pParentWnd->ActiveXY()->AreaSelectOK())
    {
      g_qeglobals.d_select_mode = sel_brush_area_partial_tall;

      Drag_Setup (x, y, buttons, xaxis, yaxis, origin, dir);
      return;
    }
  }

	// ctrl-alt-LBUTTON = multiple brush select without selecting whole entities
	if (buttons == (MK_LBUTTON | MK_CONTROL) && altdown && g_qeglobals.d_select_mode != sel_curvepoint)
	{
    nFlag = 0;
    if (sf_camera)
      nFlag |= SF_CAMERA;
    else
      nFlag |= SF_ENTITIES_FIRST;
    Select_Ray (origin, dir, nFlag);
    UpdateSurfaceDialog();

		return;
	}

	// ctrl-shift LBUTTON = select single face
	if (sf_camera && buttons == (MK_LBUTTON | MK_CONTROL | MK_SHIFT) && g_qeglobals.d_select_mode != sel_curvepoint)
	{
    if(Sys_AltDown())
    {
      brush_t *b;
      for (b=selected_brushes.next ; b != &selected_brushes ; b=b->next)
      {
        if(b->pPatch)
          continue;

        for (face_t* pFace = b->brush_faces; pFace; pFace = pFace->next)
        {
          g_ptrSelectedFaces.Add(pFace);
          g_ptrSelectedFaceBrushes.Add(b);
        }
      }

      for (b = selected_brushes.next; b != &selected_brushes; )
      {
        brush_t *pb = b;
			  b = b->next;
        Brush_RemoveFromList (pb);
        Brush_AddToList (pb, &active_brushes);
      }
    }
    else
		  Select_Deselect (true);

		Select_Ray (origin, dir, (SF_SINGLEFACE|SF_CAMERA));
		return;
	}


	// LBUTTON + all other modifiers = manipulate selection
	if (buttons & MK_LBUTTON)
	{
		Drag_Setup (x, y, buttons, xaxis, yaxis, origin, dir);
		return;
	}

	int nMouseButton = g_PrefsDlg.m_nMouseButtons == 2 ? MK_RBUTTON : MK_MBUTTON;
	// middle button = grab texture
	if (buttons == nMouseButton)
	{
		t = Test_Ray (origin, dir, false);
		if (t.face)
		{
      UpdateWorkzone_ForBrush( t.brush );
			// use a local brushprimit_texdef fitted to a default 2x2 texture
			brushprimit_texdef_t bp_local;
			ConvertTexMatWithQTexture( &t.face->brushprimit_texdef, t.face->d_texture, &bp_local, NULL );
			Texture_SetTexture ( &t.face->texdef, &bp_local, false, NULL);
			UpdateSurfaceDialog();
			UpdatePatchInspector();
		}
		else
			Sys_Printf ("Did not select a texture\n");
		return;
	}

	// ctrl-middle button = set entire brush to texture
	if (buttons == (nMouseButton|MK_CONTROL) )
	{
		t = Test_Ray (origin, dir, false);
		if (t.brush)
		{
			if (t.brush->brush_faces->texdef.GetName()[0] == '(')
				Sys_Printf ("Can't change an entity texture\n");
			else
			{
				Brush_SetTexture (t.brush, &g_qeglobals.d_texturewin.texdef, &g_qeglobals.d_texturewin.brushprimit_texdef, false, static_cast<IPluginTexdef *>( g_qeglobals.d_texturewin.pTexdef ) );
				Sys_UpdateWindows (W_ALL);
			}
		}
		else
			Sys_Printf ("Didn't hit a btrush\n");
		return;
	}

	// ctrl-shift-middle button = set single face to texture
	if (buttons == (nMouseButton|MK_SHIFT|MK_CONTROL) )
	{
		t = Test_Ray (origin, dir, false);
		if (t.brush)
		{
			if (t.brush->brush_faces->texdef.GetName()[0] == '(')
				Sys_Printf ("Can't change an entity texture\n");
			else
			{
				SetFaceTexdef (t.face, &g_qeglobals.d_texturewin.texdef, &g_qeglobals.d_texturewin.brushprimit_texdef);
				Brush_Build( t.brush );

				Sys_UpdateWindows (W_ALL);
			}
		}
		else
			Sys_Printf ("Didn't hit a btrush\n");
		return;
	}

	if (buttons == (nMouseButton | MK_SHIFT))
	{
		Sys_Printf("Set brush face texture info\n");
		t = Test_Ray (origin, dir, false);
		if (t.brush)
		{
			if (t.brush->brush_faces->texdef.GetName()[0] == '(')
      {
        if (t.brush->owner->eclass->nShowFlags & ECLASS_LIGHT)
        {
          CString strBuff;
          qtexture_t* pTex = g_qeglobals.d_texturewin.pShader->getTexture();
          if (pTex)
          {
            vec3_t vColor;
            VectorCopy(pTex->color, vColor);

            float fLargest = 0.0f;
            for (int i = 0; i < 3; i++)
            {
		          if (vColor[i] > fLargest)
			          fLargest = vColor[i];
            }

		        if (fLargest == 0.0f)
		        {
              vColor[0] = vColor[1] = vColor[2] = 1.0f;
            }
		        else
		        {
			        float fScale = 1.0f / fLargest;
              for (int i = 0; i < 3; i++)
              {
                vColor[i] *= fScale;
              }
            }
            strBuff.Format("%f %f %f",pTex->color[0], pTex->color[1], pTex->color[2]);
            SetKeyValue(t.brush->owner, "_color", strBuff.GetBuffer());
				    Sys_UpdateWindows (W_ALL);
          }
        }
        else
        {
				  Sys_Printf ("Can't select an entity brush face\n");
        }
      }
			else
			{
        Face_SetShader(t.face, g_qeglobals.d_texturewin.texdef.GetName());
				Brush_Build(t.brush);

				Sys_UpdateWindows (W_ALL);
			}
		}
		else
			Sys_Printf ("Didn't hit a brush\n");
		return;
	}

}


//
//===========
//MoveSelection
//===========
//
void MoveSelection (vec3_t move)
{
	int		i, success;
	brush_t	*b;
	CString strStatus;
	vec3_t vTemp, vTemp2, end;

	if (!move[0] && !move[1] && !move[2])
  {
		return;
  }

  if (!(g_qeglobals.d_select_mode == sel_area || g_qeglobals.d_select_mode == sel_areatall || g_qeglobals.d_select_mode == sel_brush_area_partial_tall))
  {
    move[0] = (g_nScaleHow & SCALE_X) ? 0 : move[0];
    move[1] = (g_nScaleHow & SCALE_Y) ? 0 : move[1];
    move[2] = (g_nScaleHow & SCALE_Z) ? 0 : move[2];
  }

	if (g_pParentWnd->ActiveXY()->RotateMode() || g_bPatchBendMode)
	{
		float fDeg = -move[2];
		float fAdj = move[2];
		int nAxis = 0;
		if (g_pParentWnd->ActiveXY()->GetViewType() == XY)
		{
			fDeg = -move[1];
			fAdj = move[1];
			nAxis = 2;
		}
		else
		if (g_pParentWnd->ActiveXY()->GetViewType() == XZ)
		{
			fDeg = move[2];
			fAdj = move[2];
			nAxis = 1;
		}
		else
			nAxis = 0;

		g_pParentWnd->ActiveXY()->Rotation()[nAxis] += fAdj;
		strStatus.Format("%s x:: %.1f  y:: %.1f  z:: %.1f", (g_bPatchBendMode) ? "Bend angle" : "Rotation", g_pParentWnd->ActiveXY()->Rotation()[0], g_pParentWnd->ActiveXY()->Rotation()[1], g_pParentWnd->ActiveXY()->Rotation()[2]);
		g_pParentWnd->SetStatusText(2, strStatus);

		if (g_bPatchBendMode)
		{
			Patch_SelectBendNormal();
			Select_RotateAxis(nAxis, fDeg*2, false, true);
			Patch_SelectBendAxis();
			Select_RotateAxis(nAxis, fDeg, false, true);
		}
		else
		{
			Select_RotateAxis(nAxis, fDeg, false, true);
		}
		return;
	}

	if (g_pParentWnd->ActiveXY()->ScaleMode())
	{
		vec3_t v;
		v[0] = v[1] = v[2] = 1.0f;
		if (move[1] > 0)
		{
			v[0] = 1.1f;
			v[1] = 1.1f;
			v[2] = 1.1f;
		}
		else
			if (move[1] < 0)
		{
			v[0] = 0.9f;
			v[1] = 0.9f;
			v[2] = 0.9f;
		}

			Select_Scale((g_nScaleHow & SCALE_X) ? 1.0f : v[0],
									 (g_nScaleHow & SCALE_Y) ? 1.0f : v[1],
  								 (g_nScaleHow & SCALE_Z) ? 1.0f : v[2]);
      // is that really necessary???
		Sys_UpdateWindows (W_ALL);
		return;
	}


	vec3_t vDistance;
	VectorSubtract(pressdelta, vPressStart, vDistance);
	strStatus.Format("Distance x: %.1f  y: %.1f  z: %.1f", vDistance[0], vDistance[1], vDistance[2]);
	g_pParentWnd->SetStatusText(3, strStatus);

	//
	// dragging only a part of the selection
	//

	// this is fairly crappy way to deal with curvepoint and area selection
	// but it touches the smallest amount of code this way
	//
	if (g_qeglobals.d_num_move_points || g_qeglobals.d_select_mode == sel_vertex || g_qeglobals.d_select_mode == sel_area || g_qeglobals.d_select_mode == sel_areatall || g_qeglobals.d_select_mode == sel_brush_area_partial_tall)
	{
		//area selection
    if (g_qeglobals.d_select_mode == sel_area || g_qeglobals.d_select_mode == sel_areatall || g_qeglobals.d_select_mode == sel_brush_area_partial_tall)
		{
			VectorAdd(g_qeglobals.d_vAreaBR, move, g_qeglobals.d_vAreaBR);
			return;
		}
		//curve point selection
		if (g_qeglobals.d_select_mode == sel_curvepoint)
		{
			Patch_UpdateSelected(move);
			return;
		}
		//vertex selection
		if (g_qeglobals.d_select_mode == sel_vertex && g_PrefsDlg.m_bVertexSplit)
		{
      if(g_qeglobals.d_num_move_points) {
			  success = true;
			  for (b = selected_brushes.next; b != &selected_brushes; b = b->next)
			  {
				  success &= Brush_MoveVertex(b, g_qeglobals.d_move_points[0], move, end, true);
			  }
			  if (success)
				  VectorCopy(end, g_qeglobals.d_move_points[0]);
      }
			return;
		}
		//all other selection types
		for (i=0 ; i<g_qeglobals.d_num_move_points ; i++)
			VectorAdd (g_qeglobals.d_move_points[i], move, g_qeglobals.d_move_points[i]);
		for (b = selected_brushes.next; b != &selected_brushes; b = b->next)
		{
      bool bMoved = false;
      for(face_t *f = b->brush_faces; !bMoved && f!=NULL; f=f->next)
        for(int p=0; !bMoved && p<3; p++)
          for (i=0 ; !bMoved && i<g_qeglobals.d_num_move_points ; i++)
			      if(f->planepts[p] == g_qeglobals.d_move_points[i])
              bMoved = true;
		  if(!bMoved) continue;

			VectorCopy(b->maxs, vTemp);
			VectorSubtract(vTemp, b->mins, vTemp);
      Brush_Build(b,true,true,false,false); // don't filter
			for (i=0 ; i<3 ; i++)
			{
				if (b->mins[i] > b->maxs[i]
				|| b->maxs[i] - b->mins[i] > g_MaxBrushSize)
					break;	// dragged backwards or fucked up
			}
			if (i != 3)
				break;
			if (b->patchBrush)
			{
				VectorCopy(b->maxs, vTemp2);
				VectorSubtract(vTemp2, b->mins, vTemp2);
				VectorSubtract(vTemp2, vTemp, vTemp2);
				//if (!Patch_DragScale(b->nPatchID, vTemp2, move))
				if (!Patch_DragScale(b->pPatch, vTemp2, move))
				{
					b = NULL;
					break;
				}
			}
		}
		// if any of the brushes were crushed out of existance
		// calcel the entire move
		if (b != &selected_brushes)
		{
			Sys_Printf ("Brush dragged backwards, move canceled\n");
			for (i=0 ; i<g_qeglobals.d_num_move_points ; i++)
				VectorSubtract (g_qeglobals.d_move_points[i], move, g_qeglobals.d_move_points[i]);

			for (b=selected_brushes.next ; b != &selected_brushes ; b=b->next)
				Brush_Build(b,true,true,false,false); // don't filter
		}

	}
	else
	{
		// reset face originals from vertex edit mode
		// this is dirty, but unfortunately necessary because Brush_Build
		// can remove windings
		for (b = selected_brushes.next; b != &selected_brushes; b = b->next)
		{
			Brush_ResetFaceOriginals(b);
		}
		//
		// if there are lots of brushes selected, just translate instead
		// of rebuilding the brushes
    // NOTE: this is not actually done, but would be a good idea
		//
	  Select_Move (move);
	}
}

/*
===========
Drag_MouseMoved
===========
*/
void Drag_MouseMoved (int x, int y, int buttons)
{
  vec3_t	move, delta;
  int		i;

  // Now that we're in Drag_MouseMoved() it was just done -> record that
  drag_done = true;

  if (!buttons)
  {
    drag_ok = false;
    return;
  }
  if (!drag_ok)
    return;

  // clear along one axis
  if (buttons & MK_SHIFT && (g_PrefsDlg.m_bALTEdge && ( g_qeglobals.d_select_mode != sel_areatall && g_qeglobals.d_select_mode != sel_brush_area_partial_tall)))
  {
    drag_first = false;
    if (abs(x-pressx) > abs(y-pressy))
      y = pressy;
    else
      x = pressx;
  }

	if (g_qeglobals.d_select_mode == sel_area && g_nPatchClickedView == W_CAMERA)
	{
		camera_t *m_pCamera = g_pParentWnd->GetCamWnd()->Camera();

		// snap to window
		if( y > m_pCamera->height ) y = m_pCamera->height - 1; else if( y < 0 ) y = 0;
		if( x > m_pCamera->width ) x = m_pCamera->width - 1; else if( x < 0 ) x = 0;

		VectorSet (move, x - pressx, y - pressy, 0);
	} else
	{
		for (i=0 ; i<3 ; i++)
		{
			move[i] = drag_xvec[i]*(x - pressx)	+ drag_yvec[i]*(y - pressy);
			if (g_PrefsDlg.m_bSnap)
			{
				move[i] = floor(move[i]/g_qeglobals.d_gridsize+0.5)*g_qeglobals.d_gridsize;
			}
		}
	}

  VectorSubtract (move, pressdelta, delta);
  VectorCopy (move, pressdelta);

  MoveSelection (delta);
}

/*
===========
Drag_MouseUp
===========
*/
void Drag_MouseUp (int nButtons)
{
	Sys_Status ("Drag completed.", 0);

    // shift-LBUTTON = select entire brush
    // shift-alt-LBUTTON = drill select
    // drag_done is only false if Drag_MouseMoved() has not been called; else it
    // would have been an area selection, see below code
    if (drag_done == false && nButtons == MK_SHIFT && g_qeglobals.d_select_mode != sel_curvepoint)
    {
        int nFlag = Sys_AltDown() ? SF_CYCLE : 0;
        if (l_sf_camera)
          nFlag |= SF_CAMERA;
        else
          nFlag |= SF_ENTITIES_FIRST;
        Select_Ray(l_origin, l_dir, nFlag);
        Sys_UpdateWindows (W_ALL);
    }

  if (g_qeglobals.d_select_mode == sel_area)
  {
    Patch_SelectAreaPoints(nButtons & MK_CONTROL); // adds to selection and/or deselects selected points if ctrl is held
    g_qeglobals.d_select_mode = sel_curvepoint;
		Sys_UpdateWindows (W_ALL);
  }

  if (g_qeglobals.d_select_mode == sel_areatall || g_qeglobals.d_select_mode == sel_brush_area_partial_tall)
  {
    vec3_t mins, maxs;
    // Select_Deselect() changes d_select_mode state, but we still need it to
    // determine which is the actual type of selection the user intends to do.
    select_t orig_mode = g_qeglobals.d_select_mode;

    int nDim1 = (g_pParentWnd->ActiveXY()->GetViewType() == YZ) ? 1 : 0;
    int nDim2 = (g_pParentWnd->ActiveXY()->GetViewType() == XY) ? 1 : 2;

		// get our rectangle
    mins[nDim1] = MIN( g_qeglobals.d_vAreaTL[nDim1], g_qeglobals.d_vAreaBR[nDim1] );
    mins[nDim2] = MIN( g_qeglobals.d_vAreaTL[nDim2], g_qeglobals.d_vAreaBR[nDim2] );
    maxs[nDim1] = MAX( g_qeglobals.d_vAreaTL[nDim1], g_qeglobals.d_vAreaBR[nDim1] );
    maxs[nDim2] = MAX( g_qeglobals.d_vAreaTL[nDim2], g_qeglobals.d_vAreaBR[nDim2] );

    // deselect current selection
    if( !(nButtons & MK_CONTROL) )
      Select_Deselect();

    // select new selection
    if (orig_mode == sel_areatall)
        Select_RealCompleteTall( mins, maxs );
    if (orig_mode == sel_brush_area_partial_tall)
        Select_RealPartialTall( mins, maxs );

    Sys_UpdateWindows (W_ALL);
  }

  if (g_qeglobals.d_select_translate[0] || g_qeglobals.d_select_translate[1] || g_qeglobals.d_select_translate[2])
	{
		Select_Move (g_qeglobals.d_select_translate);
		VectorCopy (vec3_origin, g_qeglobals.d_select_translate);
		Sys_UpdateWindows (W_CAMERA);
	}

  /* note: added cleanup here, since an edge drag will leave selected vertices
           in g_qeglobals.d_num_move_points
  */
  if (  g_qeglobals.d_select_mode != sel_vertex &&
        g_qeglobals.d_select_mode != sel_curvepoint &&
        g_qeglobals.d_select_mode != sel_edge)
    g_qeglobals.d_num_move_points = 0;

  g_pParentWnd->SetStatusText(3, "");
  Undo_EndBrushList(&selected_brushes);
  Undo_End();
  UpdateSurfaceDialog();
}
