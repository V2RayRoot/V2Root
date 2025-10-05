# Configuration file for the Sphinx documentation builder.

import os
import sys

# Add parent directory to path so we can import v2root
sys.path.insert(0, os.path.abspath('..'))

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'V2ROOT'
copyright = '2025, Project V2root | Sepehr0Day'
author = 'Project V2root | Sepehr0Day'
release = '1.2.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',  # For automatic API documentation
    'sphinx.ext.viewcode',  # Add links to source code
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = []  # Remove _static since it doesn't exist

# Autodoc settings
autodoc_mock_imports = []  # Don't mock anything, we want real imports

