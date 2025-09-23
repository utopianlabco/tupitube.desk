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

require_relative 'qmake'
require 'rexml/parsers/sax2parser'
require 'rexml/sax2listener'

require_relative 'info'

module RQonf

class Test
    include REXML
    
    attr_reader :rules
    attr_reader :optional
    
    def initialize(qmake)
        @qmake = qmake        
        @optional = false
    end
    
    def run(globalConfigFile, parameters, dependencyName, dependencyData, debug)
        Info.info << "Checking for " << dependencyName << "... "
        mandatory = dependencyData["mandatory"]

        qmakeLine = ""
        libs = ""
        headers = ""
        if dependencyName == "ffmpeg"
            if parameters.hasArgument?("with-ffmpeg")
                ffmpegDir = parameters.argumentValue("with-ffmpeg")
                if !ffmpegDir.nil? && !ffmpegDir.empty?
                   ffmpegLib = ffmpegDir + "/lib"
                   ffmpegInclude = ffmpegDir + "/include"                
                   libFlags = dependencyData["lib-flags"]

                   libs = "-L#{ffmpegLib} #{libFlags}"
                   headers = ffmpegInclude
                   qmakeLine = "'LIBS += #{libs}'"
                   qmakeLine += " 'INCLUDEPATH += #{ffmpegInclude}'"
                end
            else
                libPath = dependencyData["lib-path"]
                libFlags = dependencyData["lib-flags"]
                ffmpegInclude = dependencyData["headers"]

                libs = "#{libPath} #{libFlags}"
                headers = ffmpegInclude
                qmakeLine = "'LIBS += #{libs}'"
                qmakeLine += " 'INCLUDEPATH += #{ffmpegInclude}'"
            end
        end

        if dependencyName == "quazip"
           if parameters.hasArgument?("with-quazip")
                quazipDir = parameters.argumentValue("with-quazip")
                if !quazipDir.nil? && !quazipDir.empty?
                   quazipLib = quazipDir + "/lib"
                   quazipInclude = quazipDir + "/include/quazip"
                   libFlags = dependencyData["lib-flags"]

                   libs = "-L#{quazipLib} #{libFlags}"
                   headers = quazipInclude
                   qmakeLine = "'LIBS += -L#{quazipLib} #{libFlags}'"
                   qmakeLine += " 'INCLUDEPATH += #{quazipInclude}'"
                end
            else
                libPath = dependencyData["lib-path"]
                libFlags = dependencyData["lib-flags"]
                quazipInclude = dependencyData["headers"]

                libs = "#{libPath} #{libFlags}"
                headers = quazipInclude
                qmakeLine = "'LIBS += #{libPath} #{libFlags}'"
                qmakeLine += " 'INCLUDEPATH += #{quazipInclude}'"
           end
        end

        if dependencyName == "libpng"
           if parameters.hasArgument?("with-libpng")
                pngDir = parameters.argumentValue("with-libpng")
                if !pngDir.nil? && !pngDir.empty?
                   pngLib = pngDir + "/lib"
                   pngInclude = pngDir + "/include"      
                   libFlags = dependencyData["lib-flags"]          

                   libs = "-L#{pngLib} #{libFlags}"
                   headers = pngInclude
                   qmakeLine = "'LIBS += -L#{pngLib} #{libFlags}'"
                   qmakeLine += " 'INCLUDEPATH += #{pngInclude}'"
                end
            else
                libPath = dependencyData["lib-path"]
                libFlags = dependencyData["lib-flags"]
                pngInclude = dependencyData["headers"]

                libs = "#{libPath} #{libFlags}"
                headers = pngInclude
                qmakeLine = "'LIBS += #{libPath} #{libFlags}'"
                qmakeLine += " 'INCLUDEPATH += #{pngInclude}'"
           end
        end

        if dependencyName == "libsndfile"
           if parameters.hasArgument?("with-libsndfile")
                sndDir = parameters.argumentValue("with-libsndfile")
                if !sndDir.nil? && !sndDir.empty?
                   sndLib = sndDir + "/lib"
                   sndInclude = sndDir + "/include"      
                   libFlags = dependencyData["lib-flags"]          

                   libs = "-L#{sndLib} #{libFlags}"
                   headers = sndInclude
                   qmakeLine = "'LIBS += -L#{sndLib} #{libFlags}'"
                   qmakeLine += " 'INCLUDEPATH += #{sndInclude}'"
                end
            else
                libPath = dependencyData["lib-path"]
                libFlags = dependencyData["lib-flags"]
                sndInclude = dependencyData["headers"]

                libs = "#{libPath} #{libFlags}"
                headers = sndInclude
                qmakeLine = "'LIBS += #{libPath} #{libFlags}'"
                qmakeLine += " 'INCLUDEPATH += #{sndInclude}'"
           end
        end

        if dependencyName == "zlib"
            libPath = dependencyData["lib-path"]
            libFlags = dependencyData["lib-flags"]
            zlibInclude = dependencyData["headers"]

            libs = "#{libPath} #{libFlags}"
            headers = zlibInclude
            qmakeLine = "'LIBS += #{libPath} #{libFlags}'"
            qmakeLine += " 'INCLUDEPATH += #{zlibInclude}'"
        end

        path = Dir.pwd
        dir = "#{path}/configure.tests/#{dependencyName}"
        cwd = Dir.getwd

        if File.exist?(dir)
            if File.stat(dir).directory?
                Dir.chdir(dir)
                @qmake.run(qmakeLine, true)

                if not @qmake.compile(dependencyName, debug)
                    Dir.chdir(cwd)
                    
                    print "[ \033[91mFAILED\033[0m ]\n"

                    priority = "\033[92moptional\033[0m"
                    if mandatory == true
                        priority = "\033[91mrequired\033[0m"
                    end

                    Info.info << "Priority: " << priority << "\n"
                    
                    return false
                end
            else
                Dir.chdir(cwd)
                raise QonfException.new("'#{dir}' isn't a directory!")

                return false
            end
        else
            Dir.chdir(cwd)
            raise QonfException.new("'#{dir}' doesn't exists!")

            return false
        end
        
        Dir.chdir(cwd)

        globalConfigFile.addIncludePath(headers)
        globalConfigFile.addLib(libs)
        
        print "[ \033[92mOK\033[0m ]\n"

        priority = "\033[92moptional\033[0m"
        if mandatory == true
           priority = "\033[91mrequired\033[0m" 
        end

        Info.info << "Priority: " << priority << "\n"
        print " ---\n"
        
        return true
    end

end

end #module

