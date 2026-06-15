"""Configure Sphinx to build the V2Root SDK reference documentation.

The module is imported by Sphinx during documentation builds. Its module-level
values define project identity, enabled extensions, source suffixes, templates,
and HTML theme selection; it exposes no runtime library API.
"""

# Configuration file for the Sphinx documentation builder.

import os
import sys

# Add parent directory to path so we can import v2root
sys.path.insert(0, os.path.abspath('..'))

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'V2Root Python SDK'
copyright = '2026, Project V2Root'
author = 'Project V2Root, Sepehr0Day'
version = '2.0'
release = '2.0.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',
    'sphinx.ext.viewcode',
    'sphinx.ext.intersphinx',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = []  # Remove _static since it doesn't exist

# Autodoc settings
autodoc_mock_imports = []  # Don't mock anything, we want real imports
autodoc_typehints = 'description'
napoleon_google_docstring = True
napoleon_numpy_docstring = False
intersphinx_mapping = {
    'python': ('https://docs.python.org/3', None),
}
