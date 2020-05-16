#!/usr/bin/python3
import os
import shutil
import time
import glob

def run(buildScriptFilePath='./build_rpi.sh',
        sourceConfigs='./build_batched_jmem_configs_totaltime/*.h',
        targetConfigPath = './deps/jerry/jerry-core/jmem/jmem-config.h',
        sourceOutput='./build/arm-linux/debug/bin/iotjs',
        targetOutputDir='./out/'):
    if not os.path.exists(targetOutputDir):
        os.makedirs(targetOutputDir)

    jmemConfigFiles = glob.glob(sourceConfigs)
    i = 1 
    max_i = len(jmemConfigFiles)

    for jmemConfigFile in jmemConfigFiles:
        # Get case name
        jmemConfigFilePath = os.path.abspath(jmemConfigFile)
        jmemConfigFileName = os.path.basename(jmemConfigFile)
        caseName = jmemConfigFileName[:jmemConfigFileName.index(".h")]

        print("Build {} ({}/{})".format(caseName, i, max_i))

        # Copy the config file
        shutil.copy(jmemConfigFilePath, targetConfigPath)

        # Build IoT.js
        cmdline = "/bin/bash {}".format(buildScriptFilePath)
        os.system(cmdline)

        # Copy the result IoT.js binary to the output path
        targetOutputName = "iotjs-" + caseName
        targetOutputPath = os.path.join(targetOutputDir, targetOutputName)
        shutil.copy(sourceOutput, targetOutputPath)
        print(" >> Saved on {}".format(targetOutputPath))

        i += 1

run()
print("Build Done!")
