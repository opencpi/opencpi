#!/usr/bin/env python

import sys
from omniORB import CORBA, PortableServer
import CosNaming
from ossie.cf import CF, CF__POA
import xml.dom.minidom
from xml.dom.minidom import Node

#########################################################################
# Function for displaying the appropriate menu interface
def display_menu(state, *files):
    """Function to display the appropriate menu base on current state"""
    
    print ""  

    if state == 0:
        for i in range(len(_fileSeq)):
            print "  " + str(i+1) + ": " + _fileSeq[i].name
        print "n - Install application number n"
        print "x - exit"
    
    elif state == 1:
        print "s - start application"
        print "u - uninstall application"

    elif state == 2:
        print "t - stop application"

    print "Selection: ", 


#########################################################################
#builds the device assigment sequence from the DAS xml file
def buildDevSeq(dasXML):
    das = xml.dom.minidom.parse(dasXML)
    ds = []
    deviceAssignmentTypeNodeList = das.getElementsByTagName("deviceassignmenttype")
    for node in deviceAssignmentTypeNodeList:
        componentid = node.getElementsByTagName("componentid")[0].firstChild.data
        assigndeviceid = node.getElementsByTagName("assigndeviceid")[0].firstChild.data
        ds.append( CF.DeviceAssignmentType(str(componentid),str(assigndeviceid)) )
    return ds
    
    
#########################################################################
# Installs the selected application
def install_app(app_no, domMgr, dasXML,*files):
    """Function to install selected application"""
    
    name_SAD = _fileSeq[app_no].name
    _appFacProps = []

    domMgr.installApplication(name_SAD)

    print name_SAD + " successfully installed onto Domain Manager"

    # Specify what component should be deployed on particular devices
    _devSeq = buildDevSeq(dasXML)
    
##    _devSeq = [CF.DeviceAssignmentType("ossieAssemblyController","GenericBox_dev")]
##    _devSeq.append(CF.DeviceAssignmentType("ossieModBPSKResource","GenericBox_dev"))
##    _devSeq.append(CF.DeviceAssignmentType("ossieChannelResource","GenericBox_dev"))
##    _devSeq.append(CF.DeviceAssignmentType("ossieDemodResource","GenericBox_dev"))

    _applicationFactories = domMgr._get_applicationFactories()

    print "App fact seq length = " + str(len(_applicationFactories))
    print "_applicationFactories[0]->name(): "
    print str(_applicationFactories[0]._get_name())

    try:
        app = _applicationFactories[0].create(_applicationFactories[0]._get_name(),_appFacProps,_devSeq)
    except:
        print "Exception hurled from application create."
        raise

    print "Application created. Ready to run or uninstall"

    return(app)

#########################################################################
# The main loop of the testInterface
if __name__ == "__main__":
    if len(sys.argv) !=2:
        print "Usage: inputs-> <Device Assignment Sequence file>"
        sys.exit()
    
    dasXML = sys.argv[1]
    
    orb = CORBA.ORB_init(sys.argv, CORBA.ORB_ID)
    obj = orb.resolve_initial_references("NameService")
    
    rootContext = obj._narrow(CosNaming.NamingContext)
    
    if rootContext is None:
        print "Failed to narrow the root naming context"
        sys.exit(1)
    
    name = [CosNaming.NameComponent("DomainName1",""),
            CosNaming.NameComponent("DomainManager","")]
    
    try:
        obj = rootContext.resolve(name)
    
    except:
        print "DomainManger name not found"
        sys.exit(1)
    
    domMgr = obj._narrow(CF.DomainManager)
    
    #name = [CosNaming.NameComponent("DomainName1",""),
    #        CosNaming.NameComponent("DeviceManager","")]
    #try:
    #    obj = rootContext.resolve(name)
    #except:
    #    print "DeviceManager name not found"
    #    sys.exit(1)
    #devMgr = obj._narrow(CF.DeviceManager)
    
    
    _DM_fm = domMgr._get_fileMgr()
    
    _fileSeq = _DM_fm.list(".sad.xml")
    
    maximum_number_applications = len(_fileSeq)
    
    state = 0
    ch_hit = 0
    
    while 1:
        if ch_hit != 10:
            display_menu(state,_fileSeq)
        
        ch_hit = str(raw_input())
        if len(ch_hit) > 1 and state != 0:
            ch_hit = ch_hit[:1]  #strip the first character off if longer than 1
    
        if state == 0:
            if ch_hit == 'x':
                state = 99
            elif (int(ch_hit) >= 1) and (int(ch_hit) <= maximum_number_applications):
                app = install_app(int(ch_hit) - 1, domMgr, dasXML, _fileSeq)
                state = 1
    
        elif state == 1:
            if ch_hit == 'u':
                domMgr.uninstallApplication(app._get_identifier())
                print "\nApplication uninstalled from Domain Manager"
                
                app.releaseObject()
                print "Application destroyed"
                state = 0
                ch_hit = 0
     
            elif ch_hit == 's':
                app.start()
                state = 2
    
        elif state == 2:
            if ch_hit == 't':
                app.stop()
                state = 1
    
        else:
            print "this shouldn't happen\n"
            state = 0
    
        if state == 99:
            sys.exit(0)     

