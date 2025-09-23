# encoding: UTF-8

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

require 'os'
require 'yaml'
require_relative 'test'
require_relative 'globalparams'
require_relative 'info'
require_relative 'makefile'

module RQonf

  class Configure
    def initialize(args)
      @configureOptions = {} # configure script options
      parseArgs(args)

      @testsDir = Dir.getwd
      @tests = []
      @qmake = QMake.new
      @dependencies = [] # List of dependency dirs

      setConfigureOptions()
      createLauncherFile()

      Makefile::setConfigureOptions(@configureOptions)
    end

    def loadProperties(yamlFile)
      @dependencies = YAML.load_file(yamlFile)
    end

    def hasArgument?(arg)
      @configureOptions.has_key?(arg)
    end

    def argumentValue(arg)
      @configureOptions[arg].to_s
    end

    def setTestDir(dir)
      @testsDir = dir
    end

    def appendDebianFlag
      filePath = "src/mypaint/mypaint.pro"
      newLine = "DEBIAN_OS = true\n"

      originalContent = File.read(filePath)

      File.open(filePath, "w") do |file|
        # Write the new line first, followed by the original content
        file.write(newLine)
        file.write(originalContent)
      end
    end

    def verifyQtVersion(minqtversion, verbose, qtdir)
      Info.info << "Checking for Qt >= " << minqtversion << $endl

      if @qmake.findQMake(minqtversion, verbose, qtdir)
        print " [ \033[92mOK\033[0m ]\n"
      else
        print " [ \033[91mFAILED\033[0m ]\n"
        raise QonfException.new("\033[91mInvalid Qt version\033[0m.\n   Please, upgrade to #{minqtversion} or higher (Visit: http://qt-project.org)")
      end
    end

    def updateLangFiles(debug)
      Info.info << "Updating lang files... " 
      print "[ \033[92mOK\033[0m ]\n"
      if debug == 1
        system('lrelease src/shell/data/translations/tupi_*.ts')
      else
        system('lrelease -silent src/shell/data/translations/tupi_*.ts')
      end
    end

    def createTests
      @tests.clear
      findTest(@testsDir)
    end

    def runTests(linuxDistro, globalConfigFile, parameters, debug)
      baseDir = __dir__
      configPath = File.join(baseDir, "distros", "#{linuxDistro}.yml")
      loadProperties(configPath)

      @dependencies.each do |dependencyName, dependencyData|
        isMandatory = dependencyData['mandatory']
        test = Test.new(@qmake)
        succeed = test.run(globalConfigFile, parameters, dependencyName, @dependencies[dependencyName], debug)
        if !succeed && isMandatory
           raise QonfException.new("\033[91mMissing required dependency\033[0m")
        end
      end
    end

    def createMakefiles
      Info.info << "Creating makefiles..." << $endl

      if RUBY_PLATFORM.downcase.include?("darwin")
        qmakeLine = "'CONFIG += console warn_on' 'INCLUDEPATH += /usr/local/include LIBS += -L/usr/local/lib -lavdevice -lavformat -lavfilter -lavcodec -lswresample -lswscale -lavutil -lquazip1-qt5 -lsndfile -framework CoreFoundation'"
        @qmake.run(qmakeLine, true)
      else
        @qmake.run("", true)
      end

      Info.info << "Updating makefiles and source code..." << $endl

      @makefiles = Makefile::findMakefiles(Dir.getwd)

      @makefiles.each { |makefile|
        Makefile::override(makefile)
      }
    end

    def cleanupTests
      filesToRemove = [
        "main.o",
        "Makefile",
        ".qmake.stash"
      ]

      subdirs = Dir.glob('configure.tests/**/').sort.reverse
      subdirs.each do |dirPath|
        # Removing each filename in the list
        filesToRemove.each do |filename|
          filePath = File.join(dirPath, filename)

          if File.exist?(filePath)
            begin
              File.delete(filePath)
            rescue Errno::EACCES => e
              puts "  Error deleting #{filePath}: #{e.message} (Permission denied)"
            rescue => e
              puts "  Error deleting #{filePath}: #{e.message}"
            end
          end
        end

        # Removing files with the same name as the subdirectory.
        dirNamefile = File.join(dirPath, File.basename(dirPath))
        if File.exist?(dirNamefile)
          begin
            File.delete(dirNamefile)
          rescue Errno::EACCES => e
            puts "  Error deleting #{dirNamefile}: #{e.message} (Permission denied)"
          rescue => e
            puts "  Error deleting #{dirNamefile}: #{e.message}"
          end
        end
      end 
    end

    private
    def parseArgs(args)
      optc = 0
      last_opt = ""

      while args.size > optc
        arg = args[optc].strip

        if arg =~ /^--([\w-]*)={0,1}([\W\w]*)/
          opt = $1.strip
          val = $2.strip
          @configureOptions[opt] = val 
          last_opt = opt
        else 
          # arg is an arg for option
          if not last_opt.to_s.empty? and @configureOptions[last_opt].to_s.empty?
            @configureOptions[last_opt] = arg
          else
            raise "parseArgs() - Invalid option: #{arg}"
          end
        end

        optc += 1
      end
    end

    private
    def findTest(path)
      unless File.directory?(path)
        # Assign an empty array if the path is invalid.
        @dependencies = []
        return
      end

      # Assign the result of the map operation to the instance variable.
      @dependencies = Dir.glob(File.join(path, "*/")).map do |dir_path|
        File.basename(dir_path)
      end
    end

    private
    def setConfigureOptions
      if @configureOptions['prefix'].nil? then
        @configureOptions['prefix'] = "/usr"
      end

      if @configureOptions['bindir'].nil? then
         @configureOptions['bindir'] = @configureOptions['prefix'] + "/bin"
      end

      if @configureOptions['libdir'].nil? then
        if RUBY_PLATFORM == "x86_64-linux"
           @configureOptions['libdir'] = @configureOptions['prefix'] + "/lib64/tupitube"
        else
           @configureOptions['libdir'] = @configureOptions['prefix'] + "/lib/tupitube"
        end
      elsif !@configureOptions['libdir'].end_with? "tupitube" then
            @configureOptions['libdir'] = @configureOptions['libdir'] + "/tupitube"
      end

      if @configureOptions['includedir'].nil? then
         @configureOptions['includedir'] = @configureOptions['prefix'] + "/include"
      end

      if @configureOptions['sharedir'].nil? then
        @configureOptions['sharedir'] = @configureOptions['prefix'] + "/share/tupitube"
      end
    end

    private
    def createLauncherFile()
      launcher_prefix = @configureOptions['prefix']
      launcher_sharedir = @configureOptions['sharedir']
      launcher_libdir = @configureOptions['libdir']
      launcher_rasterdir = @configureOptions['libdir'] + "/raster"
      launcher_bindir = @configureOptions['bindir']

      if @configureOptions['package-build'].nil? then
        @configureOptions['package-build'] = "/usr"
      else
        @configureOptions['package-build'] = @configureOptions['prefix']
        launcher_prefix = "/usr"
        launcher_sharedir = "/usr/share/tupitube"
        if RUBY_PLATFORM == "x86_64-linux"
           launcher_libdir = "/usr/lib64/tupitube"
        else
           launcher_libdir = "/usr/lib/tupitube"
        end
        launcher_bindir = "/usr/bin"
      end

      newfile = "#!/bin/bash\n\n"
      newfile += "export TUPITUBE_HOME=\"" + launcher_prefix + "\"\n"
      newfile += "export TUPITUBE_SHARE=\"" + launcher_sharedir + "\"\n"
      newfile += "export TUPITUBE_LIB=\"" + launcher_libdir + ":" + launcher_rasterdir + "\"\n"
      newfile += "export TUPITUBE_PLUGIN=\"" + launcher_libdir + "/plugins\"\n"
      newfile += "export TUPITUBE_BIN=\"" + launcher_bindir + "\"\n\n"

      path = ""
      unless @configureOptions['with-ffmpeg'].nil? then
        value = @configureOptions['with-ffmpeg']
        path = value + "/lib:"
      end

      unless @configureOptions['with-quazip'].nil? then
        value = @configureOptions['with-quazip']
        path += value + "/lib:"
      end

      unless @configureOptions['with-libsndfile'].nil? then
        value = @configureOptions['with-libsndfile']
        path += value + "/lib:"
      end

      if RUBY_PLATFORM.downcase.include?("darwin")
        newfile += "export DYLD_FALLBACK_LIBRARY_PATH=\"" + path + "\$\{TUPITUBE_LIB\}:\$\{TUPITUBE_PLUGIN\}:$DYLD_FALLBACK_LIBRARY_PATH\"\n\n"
        newfile += "open ${TUPITUBE_BIN}/TupiTube.app $*"
      else
        newfile += "export LD_LIBRARY_PATH=\"" + path + "\$\{TUPITUBE_LIB\}:\$\{TUPITUBE_PLUGIN\}:$LD_LIBRARY_PATH\"\n\n"
        newfile += "exec ${TUPITUBE_BIN}/tupitube.bin \"$@\""
      end

      launcher = File.open("launcher/tupitube.desk", "w") { |f|
        f << newfile
      }

      newfile = "[Desktop Entry]\n"
      # newfile += "Encoding=UTF-8\n"

      newfile += "Name=TupiTube Desk\n"
      newfile += "GenericName=2D Animation Tool\n"
      newfile += "GenericName[es]=Herramienta de Animación 2D\n"
      newfile += "GenericName[pt]=Ferramenta de Animação 2D\n"
      newfile += "GenericName[ru]=Инструмент 2D-анимации\n"
      newfile += "Exec=" + launcher_bindir + "/tupitube.desk %f\n"
      newfile += "Icon=tupitube.desk\n"
      newfile += "Type=Application\n"
      newfile += "MimeType=application/tup;\n"
      newfile += "Categories=Graphics;2DGraphics;VectorGraphics;\n"
      newfile += "Keywords=2d;animation;vector;graphics;tool;\n"
      newfile += "Comment=2D Animation Tool For Schools\n"
      newfile += "Comment[es]=Herramienta de animación 2D para escuelas\n"
      newfile += "Comment[pt]=Ferramenta de animação 2D para escolas\n"
      newfile += "Comment[ru]=Инструмент 2D-анимации для школ\n"
      newfile += "Terminal=false\n"

      File.open("launcher/tupitube.desktop", "w") { |f|
        f << newfile
      }

    end
  end
end # module
