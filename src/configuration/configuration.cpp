/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve), 
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper2 project. 
** Maintained by Marek Krejza <krejza@gmail.com>
**
** Copyright: See COPYING file that comes with this distribution
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file COPYING included in the packaging of
** this file.  
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*************************************************************************/

#include <configuration.h>

Configuration::Configuration() 
{
	read(); // read the settings or set them to the default values
};

void Configuration::read() 
{
    QSettings conf("Caligor soft", "MMapper2");
        
	//conf.setPath( QSettings::NativeFormat, QSettings::UserScope, "$HOME/.mmapper2-config" );
	
	conf.beginGroup("General");
	m_firstRun = conf.value("Run first time", TRUE).toBool();
	m_mapMode = conf.value("Map Mode", 0).toInt(); //0 play, 1 map	
	conf.endGroup();

	// read general options
	conf.beginGroup("Connection");
	m_remoteServerName = conf.value("Server name", "fire.pvv.org").toString();
	m_remotePort = conf.value("Remote port number", 4242).toInt();
	m_localPort = conf.value("Local port number", 4242).toInt();
	conf.endGroup();

	conf.beginGroup("Canvas");
	m_showUpdated = conf.value("Show updated rooms", TRUE).toBool();
	m_drawNotMappedExits = conf.value("Draw not mapped exits", TRUE).toBool();
	m_drawUpperLayersTextured = conf.value("Draw upper layers textured", FALSE).toBool();
	conf.endGroup();

	conf.beginGroup("Debug");
	m_autoLog = conf.value("Enable log", FALSE).toBool();
	m_logFileName = conf.value("Log file name", "~/mmapper.log").toString();
	conf.endGroup();

	conf.beginGroup("Auto load world");
	m_autoLoadWorld = conf.value("Auto load", FALSE ).toBool();
	m_autoLoadFileName = conf.value("File name", "arda.mm2").toString();
	conf.endGroup();

	conf.beginGroup("Parser");
	m_roomNameColor = conf.value("Room name ansi color", "[32m").toString();    
	m_roomDescColor = conf.value("Room desc ansi color", "[30m").toString();    
	
	m_IAC_prompt_parser = conf.value("Use IAC-GA prompt", TRUE).toBool();
	m_useXmlParser = conf.value("Use XML parser", FALSE).toBool();

	m_moveForcePatternsList = conf.value("Move force patterns").toStringList();
	m_moveCancelPatternsList = conf.value("Move cancel patterns").toStringList();
	m_noDescriptionPatternsList = conf.value("Non-standard room description patterns").toStringList();
	m_dynamicDescriptionPatternsList = conf.value("Dynamic room description patterns").toStringList();
	m_scoutPattern = conf.value("Scout pattern", "#<You quietly scout").toString();
    m_promptPattern = conf.value("Prompt pattern", "#>>").toByteArray();
    m_exitsPattern = conf.value("Exits pattern", "#<Exits:").toString();
    m_loginPattern = conf.value("Login pattern", "#>known? ").toByteArray();
    m_passwordPattern = conf.value("Password pattern", "#>pass phrase: ").toByteArray();
    m_menuPromptPattern = conf.value("Menu prompt pattern", "#>> ").toByteArray();
    
	m_roomDescriptionsParserType = (RoomDescriptionsParserType) conf.value("Static room description parser type", (int)(RDPT_COLOR)).toInt();
    m_minimumStaticLines = conf.value("Minimum static lines in room description", 1).toInt();
    
	conf.endGroup();

	conf.beginGroup("Mume native");
	m_brief = conf.value("Brief mode", FALSE).toBool();
	conf.endGroup();
			
	if (m_moveForcePatternsList.isEmpty())
	{
		m_moveForcePatternsList.append("#=You flee head over heels.");
		m_moveForcePatternsList.append("#=You are borne along by a strong current.");
		m_moveForcePatternsList.append("#?leads you out");
		//m_moveForcePatternsList.append("#=You are dead! Sorry...");
		m_moveForcePatternsList.append("#<Your feet slip, making you fall to the");
		m_moveForcePatternsList.append("#<Suddenly an explosion of ancient rhymes");
	}

	if (m_moveCancelPatternsList.isEmpty())
	{
		m_moveCancelPatternsList.append("#=Your boat cannot enter this place.");
		m_moveCancelPatternsList.append("#>steps in front of you.");
		m_moveCancelPatternsList.append("#>bars your way.");
		m_moveCancelPatternsList.append("#=Maybe you should get on your feet first?");
		m_moveCancelPatternsList.append("#<Alas, you cannot go that way...");
		m_moveCancelPatternsList.append("#<You need to swim");
		m_moveCancelPatternsList.append("#=You failed swimming there.");
		m_moveCancelPatternsList.append("#<You failed to climb there");
		m_moveCancelPatternsList.append("#=No way! You are fighting for your life!");
		m_moveCancelPatternsList.append("#<Nah...");
		m_moveCancelPatternsList.append("#<You are too exhausted.");
		m_moveCancelPatternsList.append("#>is too exhausted.");
		m_moveCancelPatternsList.append("#=PANIC! You couldn't escape!");
		m_moveCancelPatternsList.append("#=PANIC! You can't quit the fight!");
		m_moveCancelPatternsList.append("#<ZBLAM");
		m_moveCancelPatternsList.append("#<Oops!");
		m_moveCancelPatternsList.append("#>seems to be closed.");
		m_moveCancelPatternsList.append("#>ou need to climb to go there.");
		m_moveCancelPatternsList.append("#=In your dreams, or what?");
		m_moveCancelPatternsList.append("#<You'd better be swimming");
		m_moveCancelPatternsList.append("#=You unsuccessfully try to break through the ice.");
		m_moveCancelPatternsList.append("#=Your mount refuses to follow your orders!");
		m_moveCancelPatternsList.append("#<You are too exhausted to ride");
		m_moveCancelPatternsList.append("#=You can't go into deep water!");
		m_moveCancelPatternsList.append("#=You don't control your mount!");
		m_moveCancelPatternsList.append("#=Your mount is too sensible to attempt such a feat.");
		m_moveCancelPatternsList.append("#?* prevents you from going *.");
		m_moveCancelPatternsList.append("#=Scouting in that direction is impossible.");		
		m_moveCancelPatternsList.append("#<You stop moving towards");	
		m_moveCancelPatternsList.append("#>is too difficult.");				
	}
	
	if (m_noDescriptionPatternsList.isEmpty())
	{
		m_noDescriptionPatternsList.append("#=It is pitch black...");
		m_noDescriptionPatternsList.append("#=You just see a dense fog around you...");
	}
	
	if (m_dynamicDescriptionPatternsList.isEmpty())
	{
		m_dynamicDescriptionPatternsList.append("#!(?:A|An|The)[^.]*\\.");
		m_dynamicDescriptionPatternsList.append("#![^.]*(?:sit(?:s|ting)?|rest(?:s|ing)?|stand(?:s|ing)|sleep(?:s|ing)?|swim(?:s|ming)?|walk(?:s|ing)?|wander(?:s|ing)?|grow(?:s|ing)?|lies?|lying)[^.]*");
		m_dynamicDescriptionPatternsList.append("#!.*(?:\\(glowing\\)|\\(shrouded\\)|\\(hidden\\))\\.?");
		m_dynamicDescriptionPatternsList.append("#![^.]*(in|on) the ground\\.");
		m_dynamicDescriptionPatternsList.append("#?*");
		m_dynamicDescriptionPatternsList.append("#<You have found");
		m_dynamicDescriptionPatternsList.append("#<You have revealed");
		m_dynamicDescriptionPatternsList.append("#>whip all around you.");
		m_dynamicDescriptionPatternsList.append("#<Clusters of");
		m_dynamicDescriptionPatternsList.append("#<Prickly");
		m_dynamicDescriptionPatternsList.append("#!.*arrived from.*\\.");
	}
	
	conf.beginGroup("Path Machine");
	m_acceptBestRelative = conf.value("relative path acceptance", 10).toDouble();
	m_acceptBestAbsolute = conf.value("absolute path acceptance", 3).toDouble();
	m_newRoomPenalty = conf.value("room creation penalty", 5).toDouble();
	m_correctPositionBonus = conf.value("correct position bonus", 5.1).toDouble();
	m_multipleConnectionsPenalty = conf.value("multiple connections penalty", 2.0).toDouble();
	m_maxPaths = conf.value("maximum number of paths", 500).toInt();
	m_matchingTolerance = conf.value("room matching tolerance", 5).toInt();
	
	conf.endGroup();
};

void Configuration::write() const {

    QSettings conf("Caligor soft", "MMapper2");
//	conf.setPath( QSettings::NativeFormat, QSettings::UserScope, "$HOME/.mmapper2-config" );

	conf.beginGroup("General");
	conf.setValue("Run first time", FALSE);
	conf.setValue("Map Mode", m_mapMode);
	conf.endGroup();

	// write general options
	conf.beginGroup("Connection");
	conf.setValue("Server name", m_remoteServerName);
	conf.setValue("Remote port number", (int) m_remotePort);
	conf.setValue("Local port number", (int) m_localPort);
	conf.endGroup();

	// write style options
	conf.beginGroup("Canvas");
	conf.setValue("Show updated rooms", m_showUpdated);
	conf.setValue("Draw not mapped exits", m_drawNotMappedExits);
	conf.setValue("Draw upper layers textured", m_drawUpperLayersTextured);
	conf.endGroup();

	conf.beginGroup("Debug");
	conf.setValue("Enable log", m_autoLog);
	conf.setValue("Log file name", m_logFileName);
	conf.endGroup();

	conf.beginGroup("Auto load world");
	conf.setValue("Auto load", m_autoLoadWorld);
	conf.setValue("File name", m_autoLoadFileName);
	conf.endGroup();

	conf.beginGroup("Parser");
	conf.setValue("Room name ansi color", m_roomNameColor);
	conf.setValue("Room desc ansi color", m_roomDescColor);

	conf.setValue("Use IAC-GA prompt", m_IAC_prompt_parser);
	conf.setValue("Use XML parser", m_useXmlParser);

	conf.setValue("Move force patterns", m_moveForcePatternsList);
	conf.setValue("Move cancel patterns", m_moveCancelPatternsList);
	conf.setValue("No room description patterns", m_noDescriptionPatternsList);
	conf.setValue("Dynamic room description patterns", m_dynamicDescriptionPatternsList);
	conf.setValue("Scout pattern", m_scoutPattern);
    conf.setValue("Prompt pattern", m_promptPattern);
    conf.setValue("Exits pattern", m_exitsPattern);
    conf.setValue("Login pattern", m_loginPattern);
    conf.setValue("Password pattern", m_passwordPattern);
    conf.setValue("Menu prompt pattern", m_menuPromptPattern);

    conf.setValue("Static room description parser type", (int)m_roomDescriptionsParserType);
    conf.setValue("Minimum static lines in room description", m_minimumStaticLines);
	conf.endGroup();

	conf.beginGroup("Mume native");
	conf.setValue("Brief mode", m_brief);
	conf.endGroup();
	
	conf.beginGroup("Path Machine");
	conf.setValue("relative path acceptance", m_acceptBestRelative);
	conf.setValue("absolute path acceptance", m_acceptBestAbsolute);
	conf.setValue("room creation penalty", m_newRoomPenalty);
	conf.setValue("correct position bonus", m_correctPositionBonus);
	conf.setValue("maximum number of paths", m_maxPaths);
	conf.setValue("room matching tolerance", m_matchingTolerance);
	conf.setValue("multiple connections penalty", m_multipleConnectionsPenalty);
	conf.endGroup();
};

Configuration& Config() {
	static Configuration conf;
	return conf;
};

/*
TerrainType 	INDOORS		images/indoors.png		    [
TerrainType 	City		images/city.png		    	    #
#TerrainType 	City		images/test.jpg			    #
TerrainType	field		images/field.png	            .
TerrainType	FOREST		images/forest.png	            f
TerrainType	HILLS		images/hills.png	            (
TerrainType	MOUNTAINS	images/mountain.png	            <
TerrainType	SHALLOWWATER	images/shallowwater.png		    %
TerrainType	WATER		images/river.png                    ~
TerrainType	RAPIDS		images/rapids.png 	            W
TerrainType	UNDERWATER	images/underwater.png 	            U
TerrainType	ROAD		images/road.png   	            +
TerrainType	BRUSH		images/brush.png  	            :
TerrainType	TUNNEL		images/tunnel.png	            =
TerrainType	CAVERN		images/cavern.png  	            O
TerrainType	DEATH		images/death_128.png  	            ? 
*/
