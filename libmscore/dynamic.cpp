//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "dynamic.h"
#include "xml.h"
#include "score.h"
#include "measure.h"
#include "system.h"
#include "segment.h"
#include "utils.h"
#include "style.h"
#include "mscore.h"
#include "chord.h"
#include "undo.h"
#include "sym.h"

namespace Ms {

//-----------------------------------------------------------------------------
//   Dyn
//    see: http://en.wikipedia.org/wiki/File:Dynamic's_Note_Velocity.svg
//-----------------------------------------------------------------------------

struct Dyn {
      int velocity;      ///< associated midi velocity (0-127, -1 = none)
      bool accent;       ///< if true add velocity to current chord velocity
      const char* tag;   // name of dynamics, eg. "fff"
      const char* text;  // utf8 text of dynamic
      };

#if 0
static Dyn dynList[] = {
      // dynamic:
      {  -1,  true,  "other-dynamics", ""                                                       },
      {   1,  false, "pppppp", u8"\U0001d18f\U0001d18f\U0001d18f\U0001d18f\U0001d18f\U0001d18f" },
      {   5,  false, "ppppp",  u8"\U0001d18f\U0001d18f\U0001d18f\U0001d18f\U0001d18f"           },
      {  10,  false, "pppp",   u8"\U0001d18f\U0001d18f\U0001d18f\U0001d18f"                     },
      {  16,  false, "ppp",    u8"\U0001d18f\U0001d18f\U0001d18f"                               },
      {  33,  false, "pp",     u8"\U0001d18f\U0001d18f"                                         },
      {  49,  false, "p",      u8"\U0001d18f"                                                   },
      {  64,  false, "mp",     u8"\U0001d190\U0001d18f"                                         },
      {  80,  false, "mf",     u8"\U0001d190\U0001d191"                                         },
      {  96,  false, "f",      u8"\U0001d191"                                                   },
      { 112,  false, "ff",     u8"\U0001d191\U0001d191"                                         },
      { 126,  false, "fff",    u8"\U0001d191\U0001d191\U0001d191"                               },
      { 127,  false, "ffff",   u8"\U0001d191\U0001d191\U0001d191\U0001d191"                     },
      { 127,  false, "fffff",  u8"\U0001d191\U0001d191\U0001d191\U0001d191\U0001d191"           },
      { 127,  false, "ffffff", u8"\U0001d191\U0001d191\U0001d191\U0001d191\U0001d191\U0001d191" },

      // accents:
      {  0,   true,  "fp",     u8"\U0001d191\U0001d18f"},
      {  0,   true,  "sf",     u8"\U0001d18d\U0001d191"},
      {  0,   true,  "sfz",    u8"\U0001d18d\U0001d191\U0001d18e"},
      {  0,   true,  "sff",    u8"\U0001d18d\U0001d191\U0001d191"},
      {  0,   true,  "sffz",   u8"\U0001d18d\U0001d191\U0001d191\U0001d18e"},
      {  0,   true,  "sfp",    u8"\U0001d18d\U0001d191\U0001d18f"},
      {  0,   true,  "sfpp",   u8"\U0001d18d\U0001d191\U0001d18f\U0001d18f"},
      {  0,   true,  "rfz",    u8"\U0001d18c\U0001d191\U0001d18e"},
      {  0,   true,  "rf",     u8"\U0001d18c\U0001d191"},
      {  0,   true,  "fz",     u8"\U0001d191\U0001d18e"},
      {  0,   true,  "m",      u8"\U0001d190"},
      {  0,   true,  "r",      u8"\U0001d18c"},
      {  0,   true,  "s",      u8"\U0001d18d"},
      {  0,   true,  "z",      u8"\U0001d18e"},
      };
#endif

// bravura version:

static Dyn dynList[] = {
      // dynamic:
      {  -1,  true,  "other-dynamics", ""     },
      {   1,  false, "pppppp", u8"\U0000e4e7" },
      {   5,  false, "ppppp",  u8"\U0000e4e8" },
      {  10,  false, "pppp",   u8"\U0000e4e9" },
      {  16,  false, "ppp",    u8"\U0000e4ea" },
      {  33,  false, "pp",     u8"\U0000e4eb" },
      {  49,  false, "p",      u8"\U0000e4e0" },
      {  64,  false, "mp",     u8"\U0000e4ec" },
      {  80,  false, "mf",     u8"\U0000e4ed" },
      {  96,  false, "f",      u8"\U0000e4e2" },
      { 112,  false, "ff",     u8"\U0000e4ef" },
      { 126,  false, "fff",    u8"\U0000e4f0" },
      { 127,  false, "ffff",   u8"\U0000e4f1" },
      { 127,  false, "fffff",  u8"\U0000e4f2" },
      { 127,  false, "ffffff", u8"\U0000e4f3" },

      // accents:
      {  0,   true,  "fp",     u8"\U0000e4f4" },
      {  0,   true,  "sf",     u8"\U0000e4f6" },
      {  0,   true,  "sfz",    u8"\U0000e4f9"},
      {  0,   true,  "sff",    u8"\U0000e4f6\U0000e4e2"},
      {  0,   true,  "sffz",   u8"\U0000e4fa"},
      {  0,   true,  "sfp",    u8"\U0000e4f7"},
      {  0,   true,  "sfpp",   u8"\U0000e4f8"},
      {  0,   true,  "rfz",    u8"\U0000e4fc"},
      {  0,   true,  "rf",     u8"\U0000e4fb"},
      {  0,   true,  "fz",     u8"\U0000e4f5"},
      {  0,   true,  "m",      u8"\U0000e4e1"},
      {  0,   true,  "r",      u8"\U0000e4e3"},
      {  0,   true,  "s",      u8"\U0000e4e4"},
      {  0,   true,  "z",      u8"\U0000e4e5"},
      {  0,   true,  "n",      u8"\U0000e4e6"}
      };

//---------------------------------------------------------
//   Dynamic
//---------------------------------------------------------

Dynamic::Dynamic(Score* s)
   : Text(s)
      {
      // setFlags(ELEMENT_MOVABLE | ELEMENT_SELECTABLE | ELEMENT_ON_STAFF);
      setFlags(ELEMENT_MOVABLE | ELEMENT_SELECTABLE);
      _velocity = -1;
      _dynRange = DYNAMIC_PART;
      setTextStyleType(TEXT_STYLE_DYNAMICS);
      _dynamicType  = DYNAMIC_OTHER;
      }

Dynamic::Dynamic(const Dynamic& d)
   : Text(d)
      {
      _dynamicType = d._dynamicType;
      _velocity    = d._velocity;
      _dynRange    = d._dynRange;
      }

//---------------------------------------------------------
//   setVelocity
//---------------------------------------------------------

void Dynamic::setVelocity(int v)
      {
      _velocity = v;
      }

//---------------------------------------------------------
//   velocity
//---------------------------------------------------------

int Dynamic::velocity() const
      {
      return _velocity <= 0 ? dynList[dynamicType()].velocity : _velocity;
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Dynamic::write(Xml& xml) const
      {
      xml.stag("Dynamic");
      xml.tag("subtype", dynamicTypeName());
      writeProperty(xml, P_VELOCITY);
      writeProperty(xml, P_DYNAMIC_RANGE);
      Text::writeProperties(xml, dynamicType() == 0);
      xml.etag();
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Dynamic::read(XmlReader& e)
      {
      while (e.readNextStartElement()) {
            const QStringRef& tag = e.name();
            if (tag == "subtype") {
                  setDynamicType(e.readElementText());
                  }
            else if (tag == "velocity")
                  _velocity = e.readInt();
            else if (tag == "dynType")
                  _dynRange = DynamicRange(e.readInt());
            else if (!Text::readProperties(e))
                  e.unknown();
            }
      setTextStyleType(TEXT_STYLE_DYNAMICS);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Dynamic::layout()
      {
      if (!readPos().isNull()) {
            if (score()->mscVersion() < 118) {
                  setReadPos(QPointF());
                  // hack: 1.2 boundingBoxes are a bit wider which results
                  // in symbols moved right
                  setUserXoffset(userOff().x() - spatium() * .6);
                  }
            }
      Text::layout();

      Segment* s = segment();
      if (!s)
            return;
      for (int voice = 0; voice < VOICES; ++voice) {
            int t = (track() & ~0x3) + voice;
            Chord* c = static_cast<Chord*>(s->element(t));
            if (!c)
                  continue;
            if (c->type() == CHORD) {
                  qreal noteHeadWidth = score()->noteHeadWidth() * c->mag();
                  if (c->stem() && !c->up())  // stem down
                        rxpos() += noteHeadWidth * .25;  // center on stem + optical correction
                  else
                        rxpos() += noteHeadWidth * .5;   // center on note head
                  }
            else
                  rxpos() += c->width() * .5;
            break;
            }
      }

//---------------------------------------------------------
//   setDynamicType
//---------------------------------------------------------

void Dynamic::setDynamicType(const QString& tag)
      {
      int n = sizeof(dynList)/sizeof(*dynList);
      for (int i = 0; i < n; ++i) {
            if (dynList[i].tag == tag) {
                  setDynamicType(DynamicType(i));
                  setText(QString::fromUtf8(dynList[i].text));
                  return;
                  }
            }
      qDebug("setDynamicType: other <%s>", qPrintable(tag));
      setDynamicType(DYNAMIC_OTHER);
      setText(tag);
      }

//---------------------------------------------------------
//   dynamicTypeName
//---------------------------------------------------------

QString Dynamic::dynamicTypeName() const
      {
      return dynList[dynamicType()].tag;
      }

//---------------------------------------------------------
//   startEdit
//---------------------------------------------------------

void Dynamic::startEdit(MuseScoreView* v, const QPointF& p)
      {
      Text::startEdit(v, p);
      }

//---------------------------------------------------------
//   endEdit
//---------------------------------------------------------

void Dynamic::endEdit()
      {
      Text::endEdit();
      if (!styled() || text() != QString::fromUtf8(dynList[_dynamicType].text))
            _dynamicType = DYNAMIC_OTHER;
      }

//---------------------------------------------------------
//   reset
//---------------------------------------------------------

void Dynamic::reset()
      {
//      setDynamicType(getText());
      Text::reset();
      }

//---------------------------------------------------------
//   dragAnchor
//---------------------------------------------------------

QLineF Dynamic::dragAnchor() const
      {
      qreal xp = 0.0;
      for (Element* e = parent(); e; e = e->parent())
            xp += e->x();
      qreal yp = measure()->system()->staffYpage(staffIdx());
      QPointF p(xp, yp);
      return QLineF(p, canvasPos());
      }

//---------------------------------------------------------
//   drag
//---------------------------------------------------------

QRectF Dynamic::drag(EditData* ed)
      {
      QRectF f = Element::drag(ed);

      //
      // move anchor
      //
      Qt::KeyboardModifiers km = qApp->keyboardModifiers();
      if (km != (Qt::ShiftModifier | Qt::ControlModifier)) {
            int si;
            Segment* seg = 0;
            _score->pos2measure(ed->pos, &si, 0, &seg, 0);
            if (seg && (seg != segment() || staffIdx() != si)) {
                  QPointF pos1(pagePos());
                  score()->undo(new ChangeParent(this, seg, si));
                  setUserOff(QPointF());
                  layout();
                  QPointF pos2(pagePos());
                  setUserOff(pos1 - pos2);
                  ed->startMove = pos2;
                  }
            }
      return f;
      }

//---------------------------------------------------------
//   undoSetDynRange
//---------------------------------------------------------

void Dynamic::undoSetDynRange(DynamicRange v)
      {
      score()->undoChangeProperty(this, P_DYNAMIC_RANGE, v);
      }

//---------------------------------------------------------
//   getProperty
//---------------------------------------------------------

QVariant Dynamic::getProperty(P_ID propertyId) const
      {
      switch(propertyId) {
            case P_DYNAMIC_RANGE:     return int(_dynRange);
            case P_VELOCITY:          return velocity();
            case P_SUBTYPE:           return _dynamicType;
            default:
                  return Text::getProperty(propertyId);
            }
      }

//---------------------------------------------------------
//   setProperty
//---------------------------------------------------------

bool Dynamic::setProperty(P_ID propertyId, const QVariant& v)
      {
      switch (propertyId) {
            case P_DYNAMIC_RANGE:
                  _dynRange = DynamicRange(v.toInt());
                  break;
            case P_VELOCITY:
                  _velocity = v.toInt();
                  break;
            case P_SUBTYPE:
                  _dynamicType = DynamicType(v.toInt());
                  break;
            default:
                  if (!Text::setProperty(propertyId, v))
                        return false;
                  break;
            }
      score()->setLayoutAll(true);
      return true;
      }

//---------------------------------------------------------
//   propertyDefault
//---------------------------------------------------------

QVariant Dynamic::propertyDefault(P_ID id) const
      {
      switch(id) {
            case P_TEXT_STYLE:    return TEXT_STYLE_DYNAMICS;
            case P_DYNAMIC_RANGE: return DYNAMIC_PART;
            case P_VELOCITY:      return -1;
            default:              return Text::propertyDefault(id);
            }
      }

}

