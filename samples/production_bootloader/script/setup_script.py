from cx_Freeze import setup, Executable

# To build binary application, run
# $ python3 setup_script.py build

setup(name="Recovery Bootloader script",
      description="Simple tool for performing serial DFU",
      version="0.1",
      options={"build_exe": {"build_exe": "build",
                             "packages": ["intelhex", "sliplib", "serial"],
                             "include_files": [],
                             "excludes": ["tkinter"], # more packages can be excluded to optimize size of the released build/lib directory
                             "optimize": 2,
                             "replace_paths": "* "}
                            },
      executables=[Executable(script="recovery_bootloader_cli.py",
                              targetName="RecoveryBootloader.exe",
                              )]
      )
