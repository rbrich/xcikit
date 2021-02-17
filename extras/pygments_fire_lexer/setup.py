from setuptools import setup

setup(
    name='fire_lexer',
    version="0.1",
    packages=['fire_lexer'],
    entry_points=
    """
    [pygments.lexers]
    fire_lexer = fire_lexer.lexer:FireLexer
    """,
)
