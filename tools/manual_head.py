#!/usr/bin/env python3
"""Builds docs/Manticore_User_Manual.pdf from the text below, docs/presets.json and the screenshots in docs/img."""
import json, os
from PIL import Image
from reportlab.lib.pagesizes import letter
from reportlab.lib.units import inch
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT, TA_CENTER
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Image as RLImage, Table,
                                TableStyle, PageBreak, KeepTogether, NextPageTemplate, CondPageBreak)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = os.path.join(ROOT, 'Resources', 'Fonts')
IMG = os.path.join(ROOT, 'docs', 'img')
OUT = os.path.join(ROOT, 'docs', 'Manticore_User_Manual.pdf')

pdfmetrics.registerFont(TTFont('Cinzel', os.path.join(FONTS, 'Cinzel-Bold.ttf')))
pdfmetrics.registerFont(TTFont('Raj', os.path.join(FONTS, 'Rajdhani-Medium.ttf')))
pdfmetrics.registerFont(TTFont('RajSemi', os.path.join(FONTS, 'Rajdhani-SemiBold.ttf')))
pdfmetrics.registerFont(TTFont('RajBold', os.path.join(FONTS, 'Rajdhani-Bold.ttf')))
pdfmetrics.registerFontFamily('Raj', normal='Raj', bold='RajBold', italic='Raj', boldItalic='RajBold')

TEAL = colors.HexColor('#0f9f92')
TEAL_BRIGHT = colors.HexColor('#1fd0bf')
AMBER = colors.HexColor('#e8941a')
AMBER_BRIGHT = colors.HexColor('#ffb33f')
INK = colors.HexColor('#1b2529')
DIM = colors.HexColor('#5d7275')
BG = colors.HexColor('#0a1013')
PALE = colors.HexColor('#eef6f5')
PALE_AMBER = colors.HexColor('#fff4e2')

S = {
    'h1': ParagraphStyle('h1', fontName='Cinzel', fontSize=21, leading=26, textColor=INK, spaceBefore=4, spaceAfter=10),
    'h2': ParagraphStyle('h2', fontName='RajBold', fontSize=15, leading=18, textColor=TEAL, spaceBefore=12, spaceAfter=4),
    'body': ParagraphStyle('body', fontName='Raj', fontSize=11.5, leading=15.5, textColor=INK, spaceAfter=7),
    'lead': ParagraphStyle('lead', fontName='Raj', fontSize=13, leading=18, textColor=INK, spaceAfter=10),
    'step': ParagraphStyle('step', fontName='Raj', fontSize=11.5, leading=15.5, textColor=INK, leftIndent=14, spaceAfter=4),
    'cap': ParagraphStyle('cap', fontName='RajSemi', fontSize=10, leading=13, textColor=DIM, alignment=TA_CENTER, spaceAfter=10),
    'cell': ParagraphStyle('cell', fontName='Raj', fontSize=10.5, leading=13.5, textColor=INK),
    'cellb': ParagraphStyle('cellb', fontName='RajBold', fontSize=10.5, leading=13.5, textColor=INK),
    'cellh': ParagraphStyle('cellh', fontName='RajBold', fontSize=10.5, leading=13, textColor=colors.white),
    'tiph': ParagraphStyle('tiph', fontName='RajBold', fontSize=10.5, leading=13, textColor=AMBER),
    'tip': ParagraphStyle('tip', fontName='Raj', fontSize=11, leading=15, textColor=INK),
}


def P(t, st='body'):
    return Paragraph(t, S[st])


def img(name, width=7.0 * inch, caption=None):
    path = os.path.join(IMG, name)
    w, h = Image.open(path).size
    out = [RLImage(path, width=width, height=width * h / w)]
    if caption:
        out.append(P(caption, 'cap'))
    return out


def crop(name, box, out_name):
    im = Image.open(os.path.join(IMG, name)).crop(box)
    im.save(os.path.join(IMG, out_name))
    return out_name


def tipbox(title, text, color=PALE, edge=TEAL):
    t = Table([[P(title, 'tiph')], [P(text, 'tip')]], colWidths=[7.0 * inch])
    t.setStyle(TableStyle([('BACKGROUND', (0, 0), (-1, -1), color), ('LINEBEFORE', (0, 0), (0, -1), 3, edge),
                           ('LEFTPADDING', (0, 0), (-1, -1), 12), ('RIGHTPADDING', (0, 0), (-1, -1), 12),
                           ('TOPPADDING', (0, 0), (-1, 0), 8), ('BOTTOMPADDING', (0, -1), (-1, -1), 9),
                           ('TOPPADDING', (0, 1), (-1, -1), 2)]))
    return [Spacer(1, 4), t, Spacer(1, 8)]


def table(rows, widths, head=True):
    data = []
    for i, r in enumerate(rows):
        if i == 0 and head:
            data.append([P(c, 'cellh') for c in r])
        else:
            data.append([P(r[0], 'cellb')] + [P(c, 'cell') for c in r[1:]])
    t = Table(data, colWidths=widths, repeatRows=1 if head else 0)
    st = [('VALIGN', (0, 0), (-1, -1), 'TOP'), ('LEFTPADDING', (0, 0), (-1, -1), 6), ('RIGHTPADDING', (0, 0), (-1, -1), 6),
          ('TOPPADDING', (0, 0), (-1, -1), 4), ('BOTTOMPADDING', (0, 0), (-1, -1), 5),
          ('LINEBELOW', (0, 0), (-1, -1), 0.4, colors.HexColor('#d5e2e1'))]
    if head:
        st += [('BACKGROUND', (0, 0), (-1, 0), INK)]
    for i in range(1 if head else 0, len(data)):
        if i % 2 == 0:
            st.append(('BACKGROUND', (0, i), (-1, i), colors.HexColor('#f5f9f9')))
    t.setStyle(TableStyle(st))
    return [t, Spacer(1, 8)]


# ------------------------------------------------------------------ page templates
def cover(c, doc):
    W, H = letter
    c.setFillColor(BG)
    c.rect(0, 0, W, H, fill=1, stroke=0)
    icon = os.path.join(ROOT, 'Resources', 'icon.png')
    c.drawImage(icon, W / 2 - 1.4 * inch, H - 4.6 * inch, 2.8 * inch, 2.8 * inch, mask='auto')
    c.setFillColor(colors.HexColor('#dce8e5'))
    c.setFont('Cinzel', 44)
    c.drawCentredString(W / 2, H - 5.55 * inch, 'MANTICORE')
    c.setFillColor(AMBER_BRIGHT)
    c.setFont('RajBold', 15)
    c.drawCentredString(W / 2, H - 5.95 * inch, 'MODULAR CHIMERA SYNTH  \u00b7  FELLOWSHIP OF THE STRING')
    c.setStrokeColor(TEAL_BRIGHT)
    c.setLineWidth(1)
    c.line(W / 2 - 1.6 * inch, H - 6.25 * inch, W / 2 + 1.6 * inch, H - 6.25 * inch)
    c.setFillColor(colors.HexColor('#86a09f'))
    c.setFont('RajSemi', 16)
    c.drawCentredString(W / 2, H - 6.65 * inch, 'User Manual  \u00b7  Version 1.0')
    c.setFont('Raj', 12)
    c.drawCentredString(W / 2, 1.0 * inch, 'Modular grid synthesizer for Windows VST3 hosts')
