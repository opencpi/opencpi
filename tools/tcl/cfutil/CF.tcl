#
# This file was automatically generated from CF.idl
# by idl2tcl. Do not edit.
#

package require combat

combat::ir add \
{{module {IDL:CF:1.0 CF 1.0} {{interface {IDL:CF/LifeCycle:1.0 LifeCycle\
1.0}} {interface {IDL:CF/PortSupplier:1.0 PortSupplier 1.0}} {interface\
{IDL:CF/PropertySet:1.0 PropertySet 1.0}} {interface\
{IDL:CF/TestableObject:1.0 TestableObject 1.0}} {interface\
{IDL:CF/Resource:1.0 Resource 1.0}} {struct {IDL:CF/DataType:1.0 DataType\
1.0}} {typedef {IDL:CF/Properties:1.0 Properties 1.0} {sequence\
IDL:CF/DataType:1.0}} {interface {IDL:CF/AggregateDevice:1.0 AggregateDevice\
1.0}} {interface {IDL:CF/Device:1.0 Device 1.0} {IDL:CF/LifeCycle:1.0\
IDL:CF/PortSupplier:1.0 IDL:CF/PropertySet:1.0 IDL:CF/TestableObject:1.0\
IDL:CF/Resource:1.0} {{exception {IDL:CF/Device/InvalidState:1.0 InvalidState\
1.0} {{msg {string 0}}} {}} {exception {IDL:CF/Device/InvalidCapacity:1.0\
InvalidCapacity 1.0} {{msg {string 0}} {capacities IDL:CF/Properties:1.0}}\
{}} {enum {IDL:CF/Device/AdminType:1.0 AdminType 1.0} {LOCKED SHUTTING_DOWN\
UNLOCKED}} {enum {IDL:CF/Device/OperationalType:1.0 OperationalType 1.0}\
{ENABLED DISABLED}} {enum {IDL:CF/Device/UsageType:1.0 UsageType 1.0} {IDLE\
ACTIVE BUSY}} {attribute {IDL:CF/Device/usageState:1.0 usageState 1.0}\
IDL:CF/Device/UsageType:1.0 readonly} {attribute\
{IDL:CF/Device/compositeDevice:1.0 compositeDevice 1.0}\
IDL:CF/AggregateDevice:1.0 readonly} {attribute {IDL:CF/Device/label:1.0\
label 1.0} {string 0} readonly} {attribute {IDL:CF/Device/softwareProfile:1.0\
softwareProfile 1.0} {string 0} readonly} {attribute\
{IDL:CF/Device/operationalState:1.0 operationalState 1.0}\
IDL:CF/Device/OperationalType:1.0 readonly} {attribute\
{IDL:CF/Device/adminState:1.0 adminState 1.0} IDL:CF/Device/AdminType:1.0}\
{operation {IDL:CF/Device/allocateCapacity:1.0 allocateCapacity 1.0} boolean\
{{in capacities IDL:CF/Properties:1.0}} {IDL:CF/Device/InvalidCapacity:1.0\
IDL:CF/Device/InvalidState:1.0}} {operation\
{IDL:CF/Device/deallocateCapacity:1.0 deallocateCapacity 1.0} void {{in\
capacities IDL:CF/Properties:1.0}} {IDL:CF/Device/InvalidCapacity:1.0\
IDL:CF/Device/InvalidState:1.0}}}} {enum {IDL:CF/ErrorNumberType:1.0\
ErrorNumberType 1.0} {CF_NOTSET CF_E2BIG CF_EACCES CF_EAGAIN CF_EBADF\
CF_EBADMSG CF_EBUSY CF_ECANCELED CF_ECHILD CF_EDEADLK CF_EDOM CF_EEXIST\
CF_EFAULT CF_EFBIG CF_EINPROGRESS CF_EINTR CF_EINVAL CF_EIO CF_EISDIR\
CF_EMFILE CF_EMLINK CF_EMSGSIZE CF_ENAMETOOLONG CF_ENFILE CF_ENODEV CF_ENOENT\
CF_ENOEXEC CF_ENOLCK CF_ENOMEM CF_ENOSPC CF_ENOSYS CF_ENOTDIR CF_ENOTEMPTY\
CF_ENOTSUP CF_ENOTTY CF_ENXIO CF_EPERM CF_EPIPE CF_ERANGE CF_EROFS CF_ESPIPE\
CF_ESRCH CF_ETIMEDOUT CF_EXDEV}} {typedef {IDL:CF/OctetSequence:1.0\
OctetSequence 1.0} {sequence octet}} {exception {IDL:CF/FileException:1.0\
FileException 1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0} {msg {string\
0}}} {}} {interface {IDL:CF/File:1.0 File 1.0} {} {{exception\
{IDL:CF/File/IOException:1.0 IOException 1.0} {{errorNumber\
IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}} {exception\
{IDL:CF/File/InvalidFilePointer:1.0 InvalidFilePointer 1.0} {} {}} {attribute\
{IDL:CF/File/fileName:1.0 fileName 1.0} {string 0} readonly} {attribute\
{IDL:CF/File/filePointer:1.0 filePointer 1.0} {unsigned long} readonly}\
{operation {IDL:CF/File/read:1.0 read 1.0} void {{out data\
IDL:CF/OctetSequence:1.0} {in length {unsigned long}}}\
IDL:CF/File/IOException:1.0} {operation {IDL:CF/File/setFilePointer:1.0\
setFilePointer 1.0} void {{in filePointer {unsigned long}}}\
{IDL:CF/File/InvalidFilePointer:1.0 IDL:CF/FileException:1.0}} {operation\
{IDL:CF/File/close:1.0 close 1.0} void {} IDL:CF/FileException:1.0}\
{operation {IDL:CF/File/sizeOf:1.0 sizeOf 1.0} {unsigned long} {}\
IDL:CF/FileException:1.0} {operation {IDL:CF/File/write:1.0 write 1.0} void\
{{in data IDL:CF/OctetSequence:1.0}} IDL:CF/File/IOException:1.0}}}\
{interface {IDL:CF/Resource:1.0 Resource 1.0} {IDL:CF/LifeCycle:1.0\
IDL:CF/PortSupplier:1.0 IDL:CF/PropertySet:1.0 IDL:CF/TestableObject:1.0}\
{{exception {IDL:CF/Resource/StartError:1.0 StartError 1.0} {{errorNumber\
IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}} {exception\
{IDL:CF/Resource/StopError:1.0 StopError 1.0} {{errorNumber\
IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}} {attribute\
{IDL:CF/Resource/identifier:1.0 identifier 1.0} {string 0} readonly}\
{operation {IDL:CF/Resource/start:1.0 start 1.0} void {}\
IDL:CF/Resource/StartError:1.0} {operation {IDL:CF/Resource/stop:1.0 stop\
1.0} void {} IDL:CF/Resource/StopError:1.0}}} {struct\
{IDL:CF/DeviceAssignmentType:1.0 DeviceAssignmentType 1.0}} {typedef\
{IDL:CF/DeviceAssignmentSequence:1.0 DeviceAssignmentSequence 1.0} {sequence\
IDL:CF/DeviceAssignmentType:1.0}} {interface {IDL:CF/Application:1.0\
Application 1.0} {IDL:CF/LifeCycle:1.0 IDL:CF/PortSupplier:1.0\
IDL:CF/PropertySet:1.0 IDL:CF/TestableObject:1.0 IDL:CF/Resource:1.0}\
{{struct {IDL:CF/Application/ComponentProcessIdType:1.0\
ComponentProcessIdType 1.0} {{componentId {string 0}} {processId {unsigned\
long}}} {}} {typedef {IDL:CF/Application/ComponentProcessIdSequence:1.0\
ComponentProcessIdSequence 1.0} {sequence\
IDL:CF/Application/ComponentProcessIdType:1.0}} {struct\
{IDL:CF/Application/ComponentElementType:1.0 ComponentElementType 1.0}\
{{componentId {string 0}} {elementId {string 0}}} {}} {typedef\
{IDL:CF/Application/ComponentElementSequence:1.0 ComponentElementSequence\
1.0} {sequence IDL:CF/Application/ComponentElementType:1.0}} {attribute\
{IDL:CF/Application/componentNamingContexts:1.0 componentNamingContexts 1.0}\
IDL:CF/Application/ComponentElementSequence:1.0 readonly} {attribute\
{IDL:CF/Application/name:1.0 name 1.0} {string 0} readonly} {attribute\
{IDL:CF/Application/profile:1.0 profile 1.0} {string 0} readonly} {attribute\
{IDL:CF/Application/componentImplementations:1.0 componentImplementations\
1.0} IDL:CF/Application/ComponentElementSequence:1.0 readonly} {attribute\
{IDL:CF/Application/componentDevices:1.0 componentDevices 1.0}\
IDL:CF/DeviceAssignmentSequence:1.0 readonly} {attribute\
{IDL:CF/Application/componentProcessIds:1.0 componentProcessIds 1.0}\
IDL:CF/Application/ComponentProcessIdSequence:1.0 readonly}}} {interface\
{IDL:CF/ApplicationFactory:1.0 ApplicationFactory 1.0} {} {{exception\
{IDL:CF/ApplicationFactory/CreateApplicationRequestError:1.0\
CreateApplicationRequestError 1.0} {{invalidAssignments\
IDL:CF/DeviceAssignmentSequence:1.0}} {}} {exception\
{IDL:CF/ApplicationFactory/CreateApplicationError:1.0 CreateApplicationError\
1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}}\
{exception {IDL:CF/ApplicationFactory/InvalidInitConfiguration:1.0\
InvalidInitConfiguration 1.0} {{invalidProperties IDL:CF/Properties:1.0}} {}}\
{attribute {IDL:CF/ApplicationFactory/name:1.0 name 1.0} {string 0} readonly}\
{attribute {IDL:CF/ApplicationFactory/softwareProfile:1.0 softwareProfile\
1.0} {string 0} readonly} {attribute\
{IDL:CF/ApplicationFactory/identifier:1.0 identifier 1.0} {string 0}\
readonly} {operation {IDL:CF/ApplicationFactory/create:1.0 create 1.0}\
IDL:CF/Application:1.0 {{in name {string 0}} {in initConfiguration\
IDL:CF/Properties:1.0} {in deviceAssignments\
IDL:CF/DeviceAssignmentSequence:1.0}}\
{IDL:CF/ApplicationFactory/CreateApplicationError:1.0\
IDL:CF/ApplicationFactory/InvalidInitConfiguration:1.0\
IDL:CF/ApplicationFactory/CreateApplicationRequestError:1.0}}}} {typedef\
{IDL:CF/DeviceSequence:1.0 DeviceSequence 1.0} {sequence IDL:CF/Device:1.0}}\
{interface {IDL:CF/FileSystem:1.0 FileSystem 1.0}} {exception\
{IDL:CF/InvalidObjectReference:1.0 InvalidObjectReference 1.0} {{msg {string\
0}}} {}} {interface {IDL:CF/DeviceManager:1.0 DeviceManager 1.0}\
{IDL:CF/PropertySet:1.0 IDL:CF/PortSupplier:1.0} {{struct\
{IDL:CF/DeviceManager/ServiceType:1.0 ServiceType 1.0} {{serviceObject\
Object} {serviceName {string 0}}} {}} {typedef\
{IDL:CF/DeviceManager/ServiceSequence:1.0 ServiceSequence 1.0} {sequence\
IDL:CF/DeviceManager/ServiceType:1.0}} {attribute\
{IDL:CF/DeviceManager/deviceConfigurationProfile:1.0\
deviceConfigurationProfile 1.0} {string 0} readonly} {attribute\
{IDL:CF/DeviceManager/registeredServices:1.0 registeredServices 1.0}\
IDL:CF/DeviceManager/ServiceSequence:1.0 readonly} {attribute\
{IDL:CF/DeviceManager/registeredDevices:1.0 registeredDevices 1.0}\
IDL:CF/DeviceSequence:1.0 readonly} {attribute\
{IDL:CF/DeviceManager/label:1.0 label 1.0} {string 0} readonly} {attribute\
{IDL:CF/DeviceManager/identifier:1.0 identifier 1.0} {string 0} readonly}\
{attribute {IDL:CF/DeviceManager/fileSys:1.0 fileSys 1.0}\
IDL:CF/FileSystem:1.0 readonly} {operation\
{IDL:CF/DeviceManager/registerDevice:1.0 registerDevice 1.0} void {{in\
registeringDevice IDL:CF/Device:1.0}} IDL:CF/InvalidObjectReference:1.0}\
{operation {IDL:CF/DeviceManager/getComponentImplementationId:1.0\
getComponentImplementationId 1.0} {string 0} {{in componentInstantiationId\
{string 0}}} {}} {operation {IDL:CF/DeviceManager/unregisterService:1.0\
unregisterService 1.0} void {{in unregisteringService Object} {in name\
{string 0}}} IDL:CF/InvalidObjectReference:1.0} {operation\
{IDL:CF/DeviceManager/registerService:1.0 registerService 1.0} void {{in\
registeringService Object} {in name {string 0}}}\
IDL:CF/InvalidObjectReference:1.0} {operation\
{IDL:CF/DeviceManager/shutdown:1.0 shutdown 1.0} void {} {}} {operation\
{IDL:CF/DeviceManager/unregisterDevice:1.0 unregisterDevice 1.0} void {{in\
registeredDevice IDL:CF/Device:1.0}} IDL:CF/InvalidObjectReference:1.0}}}\
{struct {IDL:CF/DataType:1.0 DataType 1.0} {{id {string 0}} {value any}} {}}\
{exception {IDL:CF/InvalidProfile:1.0 InvalidProfile 1.0} {} {}} {typedef\
{IDL:CF/StringSequence:1.0 StringSequence 1.0} {sequence {string 0}}}\
{exception {IDL:CF/UnknownProperties:1.0 UnknownProperties 1.0}\
{{invalidProperties IDL:CF/Properties:1.0}} {}} {struct\
{IDL:CF/DeviceAssignmentType:1.0 DeviceAssignmentType 1.0} {{componentId\
{string 0}} {assignedDeviceId {string 0}}} {}} {exception\
{IDL:CF/InvalidFileName:1.0 InvalidFileName 1.0} {{errorNumber\
IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}} {interface\
{IDL:CF/AggregateDevice:1.0 AggregateDevice 1.0} {} {{attribute\
{IDL:CF/AggregateDevice/devices:1.0 devices 1.0} IDL:CF/DeviceSequence:1.0\
readonly} {operation {IDL:CF/AggregateDevice/addDevice:1.0 addDevice 1.0}\
void {{in associatedDevice IDL:CF/Device:1.0}}\
IDL:CF/InvalidObjectReference:1.0} {operation\
{IDL:CF/AggregateDevice/removeDevice:1.0 removeDevice 1.0} void {{in\
associatedDevice IDL:CF/Device:1.0}} IDL:CF/InvalidObjectReference:1.0}}}\
{interface {IDL:CF/FileSystem:1.0 FileSystem 1.0} {} {{exception\
{IDL:CF/FileSystem/UnknownFileSystemProperties:1.0\
UnknownFileSystemProperties 1.0} {{invalidProperties IDL:CF/Properties:1.0}}\
{}} {const {IDL:CF/FileSystem/SIZE:1.0 SIZE 1.0} string SIZE} {const\
{IDL:CF/FileSystem/AVAILABLE_SPACE:1.0 AVAILABLE_SPACE 1.0} string\
AVAILABLE_SPACE} {enum {IDL:CF/FileSystem/FileType:1.0 FileType 1.0} {PLAIN\
DIRECTORY FILE_SYSTEM}} {struct {IDL:CF/FileSystem/FileInformationType:1.0\
FileInformationType 1.0} {{name {string 0}} {kind\
IDL:CF/FileSystem/FileType:1.0} {size {unsigned long long}} {fileProperties\
IDL:CF/Properties:1.0}} {}} {typedef\
{IDL:CF/FileSystem/FileInformationSequence:1.0 FileInformationSequence 1.0}\
{sequence IDL:CF/FileSystem/FileInformationType:1.0}} {const\
{IDL:CF/FileSystem/CREATED_TIME_ID:1.0 CREATED_TIME_ID 1.0} string\
CREATED_TIME} {const {IDL:CF/FileSystem/MODIFIED_TIME_ID:1.0 MODIFIED_TIME_ID\
1.0} string MODIFIED_TIME} {const {IDL:CF/FileSystem/LAST_ACCESS_TIME_ID:1.0\
LAST_ACCESS_TIME_ID 1.0} string LAST_ACCESS_TIME} {operation\
{IDL:CF/FileSystem/remove:1.0 remove 1.0} void {{in fileName {string 0}}}\
{IDL:CF/FileException:1.0 IDL:CF/InvalidFileName:1.0}} {operation\
{IDL:CF/FileSystem/open:1.0 open 1.0} IDL:CF/File:1.0 {{in fileName {string\
0}} {in read_Only boolean}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileException:1.0}} {operation {IDL:CF/FileSystem/create:1.0 create\
1.0} IDL:CF/File:1.0 {{in fileName {string 0}}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileException:1.0}} {operation {IDL:CF/FileSystem/list:1.0 list 1.0}\
IDL:CF/FileSystem/FileInformationSequence:1.0 {{in pattern {string 0}}}\
{IDL:CF/FileException:1.0 IDL:CF/InvalidFileName:1.0}} {operation\
{IDL:CF/FileSystem/query:1.0 query 1.0} void {{inout fileSystemProperties\
IDL:CF/Properties:1.0}} IDL:CF/FileSystem/UnknownFileSystemProperties:1.0}\
{operation {IDL:CF/FileSystem/exists:1.0 exists 1.0} boolean {{in fileName\
{string 0}}} IDL:CF/InvalidFileName:1.0} {operation\
{IDL:CF/FileSystem/rmdir:1.0 rmdir 1.0} void {{in directoryName {string 0}}}\
{IDL:CF/InvalidFileName:1.0 IDL:CF/FileException:1.0}} {operation\
{IDL:CF/FileSystem/copy:1.0 copy 1.0} void {{in sourceFileName {string 0}}\
{in destinationFileName {string 0}}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileException:1.0}} {operation {IDL:CF/FileSystem/mkdir:1.0 mkdir 1.0}\
void {{in directoryName {string 0}}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileException:1.0}}}} {interface {IDL:CF/ResourceFactory:1.0\
ResourceFactory 1.0} {} {{exception\
{IDL:CF/ResourceFactory/InvalidResourceId:1.0 InvalidResourceId 1.0} {} {}}\
{exception {IDL:CF/ResourceFactory/ShutdownFailure:1.0 ShutdownFailure 1.0}\
{{msg {string 0}}} {}} {exception\
{IDL:CF/ResourceFactory/CreateResourceFailure:1.0 CreateResourceFailure 1.0}\
{{errorNumber IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}} {attribute\
{IDL:CF/ResourceFactory/identifier:1.0 identifier 1.0} {string 0} readonly}\
{operation {IDL:CF/ResourceFactory/createResource:1.0 createResource 1.0}\
IDL:CF/Resource:1.0 {{in resourceId {string 0}} {in qualifiers\
IDL:CF/Properties:1.0}} IDL:CF/ResourceFactory/CreateResourceFailure:1.0}\
{operation {IDL:CF/ResourceFactory/shutdown:1.0 shutdown 1.0} void {}\
IDL:CF/ResourceFactory/ShutdownFailure:1.0} {operation\
{IDL:CF/ResourceFactory/releaseResource:1.0 releaseResource 1.0} void {{in\
resourceId {string 0}}} IDL:CF/ResourceFactory/InvalidResourceId:1.0}}}\
{interface {IDL:CF/FileManager:1.0 FileManager 1.0} IDL:CF/FileSystem:1.0\
{{struct {IDL:CF/FileManager/MountType:1.0 MountType 1.0} {{mountPoint\
{string 0}} {fs IDL:CF/FileSystem:1.0}} {}} {typedef\
{IDL:CF/FileManager/MountSequence:1.0 MountSequence 1.0} {sequence\
IDL:CF/FileManager/MountType:1.0}} {exception\
{IDL:CF/FileManager/NonExistentMount:1.0 NonExistentMount 1.0} {} {}}\
{exception {IDL:CF/FileManager/InvalidFileSystem:1.0 InvalidFileSystem 1.0}\
{} {}} {exception {IDL:CF/FileManager/MountPointAlreadyExists:1.0\
MountPointAlreadyExists 1.0} {} {}} {operation {IDL:CF/FileManager/mount:1.0\
mount 1.0} void {{in mountPoint {string 0}} {in file_System\
IDL:CF/FileSystem:1.0}} {IDL:CF/InvalidFileName:1.0\
IDL:CF/FileManager/MountPointAlreadyExists:1.0\
IDL:CF/FileManager/InvalidFileSystem:1.0}} {operation\
{IDL:CF/FileManager/getMounts:1.0 getMounts 1.0}\
IDL:CF/FileManager/MountSequence:1.0 {} {}} {operation\
{IDL:CF/FileManager/unmount:1.0 unmount 1.0} void {{in mountPoint {string\
0}}} IDL:CF/FileManager/NonExistentMount:1.0}}} {interface {IDL:CF/Port:1.0\
Port 1.0} {} {{exception {IDL:CF/Port/InvalidPort:1.0 InvalidPort 1.0}\
{{errorCode {unsigned short}} {msg {string 0}}} {}} {exception\
{IDL:CF/Port/OccupiedPort:1.0 OccupiedPort 1.0} {} {}} {operation\
{IDL:CF/Port/connectPort:1.0 connectPort 1.0} void {{in connection Object}\
{in connectionId {string 0}}} {IDL:CF/Port/InvalidPort:1.0\
IDL:CF/Port/OccupiedPort:1.0}} {operation {IDL:CF/Port/disconnectPort:1.0\
disconnectPort 1.0} void {{in connectionId {string 0}}}\
IDL:CF/Port/InvalidPort:1.0}}} {interface {IDL:CF/LifeCycle:1.0 LifeCycle\
1.0} {} {{exception {IDL:CF/LifeCycle/InitializeError:1.0 InitializeError\
1.0} {{errorMessages IDL:CF/StringSequence:1.0}} {}} {exception\
{IDL:CF/LifeCycle/ReleaseError:1.0 ReleaseError 1.0} {{errorMessages\
IDL:CF/StringSequence:1.0}} {}} {operation {IDL:CF/LifeCycle/initialize:1.0\
initialize 1.0} void {} IDL:CF/LifeCycle/InitializeError:1.0} {operation\
{IDL:CF/LifeCycle/releaseObject:1.0 releaseObject 1.0} void {}\
IDL:CF/LifeCycle/ReleaseError:1.0}}} {interface {IDL:CF/TestableObject:1.0\
TestableObject 1.0} {} {{exception {IDL:CF/TestableObject/UnknownTest:1.0\
UnknownTest 1.0} {} {}} {operation {IDL:CF/TestableObject/runTest:1.0 runTest\
1.0} void {{in testid {unsigned long}} {inout testValues\
IDL:CF/Properties:1.0}} {IDL:CF/TestableObject/UnknownTest:1.0\
IDL:CF/UnknownProperties:1.0}}}} {interface {IDL:CF/PropertySet:1.0\
PropertySet 1.0} {} {{exception {IDL:CF/PropertySet/InvalidConfiguration:1.0\
InvalidConfiguration 1.0} {{msg {string 0}} {invalidProperties\
IDL:CF/Properties:1.0}} {}} {exception\
{IDL:CF/PropertySet/PartialConfiguration:1.0 PartialConfiguration 1.0}\
{{invalidProperties IDL:CF/Properties:1.0}} {}} {operation\
{IDL:CF/PropertySet/configure:1.0 configure 1.0} void {{in configProperties\
IDL:CF/Properties:1.0}} {IDL:CF/PropertySet/InvalidConfiguration:1.0\
IDL:CF/PropertySet/PartialConfiguration:1.0}} {operation\
{IDL:CF/PropertySet/query:1.0 query 1.0} void {{inout configProperties\
IDL:CF/Properties:1.0}} IDL:CF/UnknownProperties:1.0}}} {interface\
{IDL:CF/DomainManager:1.0 DomainManager 1.0} IDL:CF/PropertySet:1.0\
{{exception {IDL:CF/DomainManager/ApplicationInstallationError:1.0\
ApplicationInstallationError 1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0}\
{msg {string 0}}} {}} {exception\
{IDL:CF/DomainManager/ApplicationAlreadyInstalled:1.0\
ApplicationAlreadyInstalled 1.0} {} {}} {typedef\
{IDL:CF/DomainManager/ApplicationSequence:1.0 ApplicationSequence 1.0}\
{sequence IDL:CF/Application:1.0}} {typedef\
{IDL:CF/DomainManager/ApplicationFactorySequence:1.0\
ApplicationFactorySequence 1.0} {sequence IDL:CF/ApplicationFactory:1.0}}\
{typedef {IDL:CF/DomainManager/DeviceManagerSequence:1.0\
DeviceManagerSequence 1.0} {sequence IDL:CF/DeviceManager:1.0}} {exception\
{IDL:CF/DomainManager/InvalidIdentifier:1.0 InvalidIdentifier 1.0} {} {}}\
{exception {IDL:CF/DomainManager/DeviceManagerNotRegistered:1.0\
DeviceManagerNotRegistered 1.0} {} {}} {exception\
{IDL:CF/DomainManager/ApplicationUninstallationError:1.0\
ApplicationUninstallationError 1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0}\
{msg {string 0}}} {}} {exception {IDL:CF/DomainManager/RegisterError:1.0\
RegisterError 1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0} {msg {string\
0}}} {}} {exception {IDL:CF/DomainManager/UnregisterError:1.0 UnregisterError\
1.0} {{errorNumber IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}}\
{exception {IDL:CF/DomainManager/AlreadyConnected:1.0 AlreadyConnected 1.0}\
{} {}} {exception {IDL:CF/DomainManager/InvalidEventChannelName:1.0\
InvalidEventChannelName 1.0} {} {}} {exception\
{IDL:CF/DomainManager/NotConnected:1.0 NotConnected 1.0} {} {}} {attribute\
{IDL:CF/DomainManager/domainManagerProfile:1.0 domainManagerProfile 1.0}\
{string 0} readonly} {attribute {IDL:CF/DomainManager/identifier:1.0\
identifier 1.0} {string 0} readonly} {attribute\
{IDL:CF/DomainManager/fileMgr:1.0 fileMgr 1.0} IDL:CF/FileManager:1.0\
readonly} {attribute {IDL:CF/DomainManager/applicationFactories:1.0\
applicationFactories 1.0} IDL:CF/DomainManager/ApplicationFactorySequence:1.0\
readonly} {attribute {IDL:CF/DomainManager/applications:1.0 applications 1.0}\
IDL:CF/DomainManager/ApplicationSequence:1.0 readonly} {attribute\
{IDL:CF/DomainManager/deviceManagers:1.0 deviceManagers 1.0}\
IDL:CF/DomainManager/DeviceManagerSequence:1.0 readonly} {operation\
{IDL:CF/DomainManager/registerDevice:1.0 registerDevice 1.0} void {{in\
registeringDevice IDL:CF/Device:1.0} {in registeredDeviceMgr\
IDL:CF/DeviceManager:1.0}} {IDL:CF/InvalidObjectReference:1.0\
IDL:CF/DomainManager/RegisterError:1.0\
IDL:CF/DomainManager/DeviceManagerNotRegistered:1.0\
IDL:CF/InvalidProfile:1.0}} {operation\
{IDL:CF/DomainManager/uninstallApplication:1.0 uninstallApplication 1.0} void\
{{in applicationId {string 0}}} {IDL:CF/DomainManager/InvalidIdentifier:1.0\
IDL:CF/DomainManager/ApplicationUninstallationError:1.0}} {operation\
{IDL:CF/DomainManager/installApplication:1.0 installApplication 1.0} void\
{{in profileFileName {string 0}}} {IDL:CF/InvalidProfile:1.0\
IDL:CF/DomainManager/ApplicationAlreadyInstalled:1.0\
IDL:CF/DomainManager/ApplicationInstallationError:1.0\
IDL:CF/InvalidFileName:1.0}} {operation\
{IDL:CF/DomainManager/unregisterFromEventChannel:1.0\
unregisterFromEventChannel 1.0} void {{in unregisteringId {string 0}} {in\
eventChannelName {string 0}}}\
{IDL:CF/DomainManager/InvalidEventChannelName:1.0\
IDL:CF/DomainManager/NotConnected:1.0}} {operation\
{IDL:CF/DomainManager/unregisterDevice:1.0 unregisterDevice 1.0} void {{in\
unregisteringDevice IDL:CF/Device:1.0}} {IDL:CF/InvalidObjectReference:1.0\
IDL:CF/DomainManager/UnregisterError:1.0}} {operation\
{IDL:CF/DomainManager/registerWithEventChannel:1.0 registerWithEventChannel\
1.0} void {{in registeringObject Object} {in registeringId {string 0}} {in\
eventChannelName {string 0}}} {IDL:CF/InvalidObjectReference:1.0\
IDL:CF/DomainManager/AlreadyConnected:1.0\
IDL:CF/DomainManager/InvalidEventChannelName:1.0}} {operation\
{IDL:CF/DomainManager/unregisterDeviceManager:1.0 unregisterDeviceManager\
1.0} void {{in deviceMgr IDL:CF/DeviceManager:1.0}}\
{IDL:CF/InvalidObjectReference:1.0 IDL:CF/DomainManager/UnregisterError:1.0}}\
{operation {IDL:CF/DomainManager/unregisterService:1.0 unregisterService 1.0}\
void {{in unregisteringService Object} {in name {string 0}}}\
{IDL:CF/InvalidObjectReference:1.0 IDL:CF/DomainManager/UnregisterError:1.0}}\
{operation {IDL:CF/DomainManager/registerDeviceManager:1.0\
registerDeviceManager 1.0} void {{in deviceMgr IDL:CF/DeviceManager:1.0}}\
{IDL:CF/InvalidObjectReference:1.0 IDL:CF/DomainManager/RegisterError:1.0\
IDL:CF/InvalidProfile:1.0}} {operation\
{IDL:CF/DomainManager/registerService:1.0 registerService 1.0} void {{in\
registeringService Object} {in registeredDeviceMgr IDL:CF/DeviceManager:1.0}\
{in name {string 0}}} {IDL:CF/InvalidObjectReference:1.0\
IDL:CF/DomainManager/RegisterError:1.0\
IDL:CF/DomainManager/DeviceManagerNotRegistered:1.0}}}} {interface\
{IDL:CF/PortSupplier:1.0 PortSupplier 1.0} {} {{exception\
{IDL:CF/PortSupplier/UnknownPort:1.0 UnknownPort 1.0} {} {}} {operation\
{IDL:CF/PortSupplier/getPort:1.0 getPort 1.0} Object {{in name {string 0}}}\
IDL:CF/PortSupplier/UnknownPort:1.0}}} {interface {IDL:CF/LoadableDevice:1.0\
LoadableDevice 1.0} {IDL:CF/LifeCycle:1.0 IDL:CF/PortSupplier:1.0\
IDL:CF/PropertySet:1.0 IDL:CF/TestableObject:1.0 IDL:CF/Resource:1.0\
IDL:CF/Device:1.0} {{enum {IDL:CF/LoadableDevice/LoadType:1.0 LoadType 1.0}\
{KERNEL_MODULE DRIVER SHARED_LIBRARY EXECUTABLE}} {exception\
{IDL:CF/LoadableDevice/InvalidLoadKind:1.0 InvalidLoadKind 1.0} {} {}}\
{exception {IDL:CF/LoadableDevice/LoadFail:1.0 LoadFail 1.0} {{errorNumber\
IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}} {operation\
{IDL:CF/LoadableDevice/load:1.0 load 1.0} void {{in fs IDL:CF/FileSystem:1.0}\
{in fileName {string 0}} {in loadKind IDL:CF/LoadableDevice/LoadType:1.0}}\
{IDL:CF/Device/InvalidState:1.0 IDL:CF/LoadableDevice/LoadFail:1.0\
IDL:CF/InvalidFileName:1.0 IDL:CF/LoadableDevice/InvalidLoadKind:1.0}}\
{operation {IDL:CF/LoadableDevice/unload:1.0 unload 1.0} void {{in fileName\
{string 0}}} {IDL:CF/Device/InvalidState:1.0 IDL:CF/InvalidFileName:1.0}}}}\
{interface {IDL:CF/ExecutableDevice:1.0 ExecutableDevice 1.0}\
{IDL:CF/LifeCycle:1.0 IDL:CF/PortSupplier:1.0 IDL:CF/PropertySet:1.0\
IDL:CF/TestableObject:1.0 IDL:CF/Resource:1.0 IDL:CF/Device:1.0\
IDL:CF/LoadableDevice:1.0} {{exception\
{IDL:CF/ExecutableDevice/InvalidProcess:1.0 InvalidProcess 1.0} {{errorNumber\
IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}} {exception\
{IDL:CF/ExecutableDevice/InvalidFunction:1.0 InvalidFunction 1.0} {} {}}\
{typedef {IDL:CF/ExecutableDevice/ProcessID_Type:1.0 ProcessID_Type 1.0}\
long} {exception {IDL:CF/ExecutableDevice/InvalidParameters:1.0\
InvalidParameters 1.0} {{invalidParms IDL:CF/Properties:1.0}} {}} {exception\
{IDL:CF/ExecutableDevice/InvalidOptions:1.0 InvalidOptions 1.0} {{invalidOpts\
IDL:CF/Properties:1.0}} {}} {const {IDL:CF/ExecutableDevice/STACK_SIZE_ID:1.0\
STACK_SIZE_ID 1.0} string STACK_SIZE} {const\
{IDL:CF/ExecutableDevice/PRIORITY_ID:1.0 PRIORITY_ID 1.0} string PRIORITY}\
{exception {IDL:CF/ExecutableDevice/ExecuteFail:1.0 ExecuteFail 1.0}\
{{errorNumber IDL:CF/ErrorNumberType:1.0} {msg {string 0}}} {}} {operation\
{IDL:CF/ExecutableDevice/terminate:1.0 terminate 1.0} void {{in processId\
IDL:CF/ExecutableDevice/ProcessID_Type:1.0}}\
{IDL:CF/ExecutableDevice/InvalidProcess:1.0 IDL:CF/Device/InvalidState:1.0}}\
{operation {IDL:CF/ExecutableDevice/execute:1.0 execute 1.0}\
IDL:CF/ExecutableDevice/ProcessID_Type:1.0 {{in name {string 0}} {in options\
IDL:CF/Properties:1.0} {in parameters IDL:CF/Properties:1.0}}\
{IDL:CF/Device/InvalidState:1.0 IDL:CF/ExecutableDevice/ExecuteFail:1.0\
IDL:CF/InvalidFileName:1.0 IDL:CF/ExecutableDevice/InvalidOptions:1.0\
IDL:CF/ExecutableDevice/InvalidParameters:1.0\
IDL:CF/ExecutableDevice/InvalidFunction:1.0}}}}}}}

