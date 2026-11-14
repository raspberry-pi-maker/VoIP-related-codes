"""Shared TextArea themes for the event detail viewer."""

from __future__ import annotations

from dataclasses import replace

from rich.style import Style
from textual.widgets.text_area import TextAreaTheme

DETAIL_THEME_NAME = "esl-grep-monokai"
DETAIL_KEY_COLOR = "#A6E22E"


def build_detail_theme() -> TextAreaTheme:
    """Build the monokai-based theme used by the event detail viewer."""

    base = TextAreaTheme.get_builtin_theme("monokai")
    if base is None:
        return TextAreaTheme(
            name=DETAIL_THEME_NAME,
            syntax_styles={
                "json.label": Style(color=DETAIL_KEY_COLOR, bold=True)
            },
        )

    syntax_styles = dict(base.syntax_styles)
    syntax_styles["json.label"] = Style(
        color=DETAIL_KEY_COLOR,
        bold=True,
    )
    return replace(base, name=DETAIL_THEME_NAME, syntax_styles=syntax_styles)
