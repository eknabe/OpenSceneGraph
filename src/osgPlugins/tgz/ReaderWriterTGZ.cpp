#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <osg/Geode>
#include <osg/Group>
#include <osg/Notify>

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/Registry>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <direct.h>
#else
#include <unistd.h>
#endif

using namespace osg;

class ReaderWriterTGZ : public osgDB::ReaderWriter
{
    public:
        virtual const char* className() { return "TGZ Database Reader/Writer"; }

        virtual bool acceptsExtension(const std::string& extension)
        {
            return osgDB::equalCaseInsensitive(extension,"tgz");
        }

        virtual ReadResult readNode(const std::string& fileName, const osgDB::ReaderWriter::Options*)
        {
            std::string ext = osgDB::getLowerCaseFileExtension(fileName);
            if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

            osg::notify(osg::INFO)<<   "ReaderWriterTGZ::readNode( "<<fileName.c_str()<<" )\n";

            char dirname[128];
            char command[1024];

		#if defined(_WIN32) && !defined(__CYGWIN__)
            strcpy(dirname, "C:/Windows/Temp/.osgdb_tgz");
            mkdir( dirname);
            // note, the following C option under windows does not seem to work...
            // will pursue an better tar.exe later. RO.
            sprintf( command,
                "tar xfz %s %s",
                fileName.c_str(), dirname );
        #endif

        #if defined(__linux) || defined(__CYGWIN__)
            sprintf( dirname, "/tmp/.tgz%06d", getpid());
            mkdir( dirname, 0700 );
            sprintf( command,
                "tar xfCz %s %s",
                fileName.c_str(), dirname );
        #endif
        #ifdef __sgi
            sprintf( dirname, "/tmp/.tgz%06d", getpid());
            mkdir( dirname, 0700 );
            sprintf( command,
                "cp %s %s; cd %s;"
                "gzcat %s | tar xf -",
                fileName.c_str(), dirname, dirname,
                fileName.c_str());
        #endif
            system( command );

            osg::Group *grp = new osg::Group;
 
            osgDB::PushAndPopDataPath tmppath(dirname );

            // deactivate the automatic generation of images to geode's.
            bool prevCreateNodeFromImage = osgDB::Registry::instance()->getCreateNodeFromImage();
            osgDB::Registry::instance()->setCreateNodeFromImage(false);

            osgDB::DirectoryContents contents = osgDB::getDirectoryContents(dirname);
            for(osgDB::DirectoryContents::iterator itr = contents.begin();
                itr != contents.end();
                ++itr)
            {
                std::string file_ext = osgDB::getFileExtension(*itr);
                if (!acceptsExtension(file_ext) && 
                    *itr!=std::string(".") && 
                    *itr!=std::string(".."))
                {
                    osg::Node *node = osgDB::readNodeFile(*itr);
                    grp->addChild( node );
                }
            }

            // restorre original state of the automatic generation of images to geode's.
            osgDB::Registry::instance()->setCreateNodeFromImage(prevCreateNodeFromImage);

		#if defined(_WIN32) && !defined(__CYGWIN__)
            // note, is this the right command for windows?
            // is there any way of overiding the Y/N option? RO.
            sprintf( command, "erase %s", dirname );
            system( command );
        #else
            sprintf( command, "rm -rf %s", dirname );
            system( command );
        #endif

            if( grp->getNumChildren() == 0 )
            {
                grp->unref();
                return ReadResult::FILE_NOT_HANDLED;
            }

            return grp;

        }

};

// now register with sgRegistry to instantiate the above
// reader/writer.
osgDB::RegisterReaderWriterProxy<ReaderWriterTGZ> g_readerWriter_TGZ_Proxy;
