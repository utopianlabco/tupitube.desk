#!/usr/bin/ruby

###########################################################################
#   Project TupiTube Desk                                                 #
#   Project Contact: info@tupitube.com                                    #
#   Project Website: http://www.tupitube.com                              # 
#                                                                         #
#   Developers:                                                           #
#   2025:                                                                 #
#    Utopian Lab Development Team                                         #
#   2010:                                                                 #
#    Gustav Gonzalez                                                      #
#   ---                                                                   #
#   KTooN's versions:                                                     #
#   2006:                                                                 #
#    David Cuadrado                                                       #
#    Jorge Cuadrado                                                       #
#   2003:                                                                 #
#    Fernado Roldan                                                       #
#    Simena Dinas                                                         #
#                                                                         #
#   License:                                                              #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program.  If not, see <http://www.gnu.org/licenses/>. #
###########################################################################

# TODO: This script must detect if every command line given is valid 
#       Currently, it just try to check if some of them are included or not

# Note: gem install os -v 0.9.6

require 'fileutils'
require_relative 'qonf/configure'
require_relative 'qonf/info'
require_relative 'qonf/detectdistro'

begin
    configurator = RQonf::Configure.new(ARGV)
    globalConfigFile = RQonf::GlobalParams.new

    # Script options
    if configurator.hasArgument?("help") or configurator.hasArgument?("h")
       puts <<_EOH_
Use: ./configure [options]
  options:
  --help:                   Show this message
  --prefix=[path]:          Sets installation path [/usr]
  --bindir=[path]:          Set binaries path [/usr/bin]
  --libdir=[path]:          Set library path [/usr/lib/tupitube | /usr/lib64/tupitube]
  --sharedir=[path]:        Set data path [/usr/share]
  --with-ffmpeg=[path]:     Set ffmpeg installation path [/usr]
  --with-libpng=[path]:     Set libpng installation path [/usr]
  --with-quazip=[path]:     Set quazip installation path [/usr]
  --with-libsndfile=[path]: Set libsndfile installation path [/usr]
  --without-debug:          Disable debug
  --with-qtdir=[path]:      Set Qt directory [i.e. /usr/local/qt]
  --package-build:          Option exclusive for package maintainers
  --install-headers:        Include header files as part of installation
_EOH_
        exit 0
    end

    # Version values
    version = "0.2"
    codeName = "Páy"
    revision = "23"
    configVersion = "5"

    if configurator.hasArgument?("with-ffmpeg") and configurator.hasArgument?("without-ffmpeg")
       Info.error << " ERROR: Options --with-ffmpeg and --without-ffmpeg are mutually exclusive\n"
       exit 0
    end

    debug = 1
    if configurator.hasArgument?("without-debug") # Disabling debug flag if requested
       debug = 0
    end

    Info.info << "Linux distro detected: "
    linuxDistro = RQonf::DetectDistro.getLinuxDistroID
    print "\033[92m#{linuxDistro}\033[0m\n"

    if linuxDistro == "debian"
       configurator.appendDebianFlag
    end

    Info.info << "Compiling \033[91mTupiTube " +  version + "." + revision + "\033[0m (" +  codeName + ")" << $endl

    Info.info << "Debug support... "
    if debug
       globalConfigFile.addOption("debug")
       globalConfigFile.addDefine("TUP_DEBUG")
       print "[ \033[92mON\033[0m ]\n"
    else
       globalConfigFile.addOption("release")
       globalConfigFile.addDefine("TUP_NODEBUG")
       print "[ \033[91mOFF\033[0m ]\n"
    end

    # globalConfigFile.addDefine("TUP_SERVER")

    if configurator.hasArgument?("with-qtdir")
       qtdir = configurator.argumentValue("with-qtdir")
       configurator.verifyQtVersion("5.15.0", debug, qtdir)
    else
       configurator.verifyQtVersion("5.15.0", debug, "")
    end

    # Capturing configure parameters provided by the user to store them in global_variables.pri 
    if configurator.hasArgument?("with-ffmpeg")
       ffmpegDir = configurator.argumentValue("with-ffmpeg")
       if File.directory? ffmpegDir 
          ffmpegLib = ffmpegDir + "/lib"
          ffmpegInclude = ffmpegDir + "/include"
          globalConfigFile.addLib("-L" + ffmpegLib)
          globalConfigFile.addIncludePath(ffmpegInclude)
       else
          Info.error << " ERROR: ffmpeg directory does not exist!\n"
          exit 0
       end
    end

    if configurator.hasArgument?("with-libpng")
       libpngDir = configurator.argumentValue("with-libpng")
       if File.directory? libpngDir
          libpngLib = libpngDir + "/lib"
          if RUBY_PLATFORM =~ /linux/
              libpngInclude = libpngDir + "/include"
          elsif RUBY_PLATFORM =~ /darwin/
              libpngInclude = libpngDir + "/include"
          end
          globalConfigFile.addLib("-L" + libpngLib)
	       globalConfigFile.addLib("-lpng")
          globalConfigFile.addIncludePath(libpngInclude)
       else
          Info.error << " ERROR: libpng directory does not exist!\n"
          exit 0
       end
    end

    if configurator.hasArgument?("with-quazip")
       quazipDir = configurator.argumentValue("with-quazip")
       if File.directory? quazipDir
          quazipLib = quazipDir + "/lib"
          if RUBY_PLATFORM =~ /linux/
              quazipInclude = quazipDir + "/include"
          elsif RUBY_PLATFORM =~ /darwin/
              quazipInclude = quazipDir + "/include"
          end
          globalConfigFile.addLib("-L" + quazipLib)
          globalConfigFile.addIncludePath(quazipInclude)
       else
          Info.error << " ERROR: quazip directory does not exist!\n"
          exit 0
       end
    end

    if configurator.hasArgument?("with-libsndfile")
       libsndfileDir = configurator.argumentValue("with-libsndfile")
       if File.directory? libsndfileDir
          libsndfileLib = libsndfileDir + "/lib"
          libsndfileInclude = libsndfileDir + "/include"
          globalConfigFile.addLib("-L" + libsndfileLib)
          globalConfigFile.addIncludePath(libsndfileInclude)
       else
          Info.error << " ERROR: libsndfile directory does not exist!\n"
          exit 0
       end
    end

    configurator.setTestDir("configure.tests")
    configurator.createTests
    configurator.runTests(linuxDistro, globalConfigFile, configurator, debug)
    configurator.updateLangFiles(debug)

    globalConfigFile.addModule("core")
    globalConfigFile.addModule("gui")
    globalConfigFile.addModule("svg")
    globalConfigFile.addModule("xml")
    globalConfigFile.addModule("network")

    globalConfigFile.addLib("-ltupifwgui")
    globalConfigFile.addLib("-ltupifwcore")
    
    if configurator.hasArgument?("install-headers")
       globalConfigFile.addDefine("ADD_HEADERS");
    end

    globalConfigFile.addDefine('TUPITUBE_VERSION=\\\\\"' + version + '\\\\\"')
    globalConfigFile.addDefine('CODE_NAME=\\\\\"' + codeName + '\\\\\"')
    globalConfigFile.addDefine('REVISION=\\\\\"' + revision + '\\\\\"')
    globalConfigFile.addDefine('CONFIG_VERSION=\\\\\"' + configVersion + '\\\\\"')

    unix = globalConfigFile.addScope("unix")
    unix.addVariable("MOC_DIR", ".moc")
    unix.addVariable("UI_DIR", ".ui")
    unix.addVariable("OBJECTS_DIR", ".obj")

    globalConfigFile.save("global_variables.pri")
    configurator.createMakefiles
    configurator.cleanupTests()

   rescue => err
    Info.error << "Configure failed. error was: #{err.message}\n"
    if $DEBUG
        puts err.backtrace
    end
end
