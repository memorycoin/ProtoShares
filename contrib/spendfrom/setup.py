from distutils.core import setup
setup(name='MMCspendfrom',
      version='1.0.1',
      description='Command-line utility for bitcoin "coin control"',
      author='Memorycoin Dev Team',
      author_email='admins@mmc-square.com',
      requires=['jsonrpc'],
      scripts=['spendfrom.py'],
      )
