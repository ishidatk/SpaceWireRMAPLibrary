/* 
 ============================================================================
 SpaceWire/RMAP Library is provided under the MIT License.
 ============================================================================

 Copyright (c) 2006-2013 Takayuki Yuasa and The Open-source SpaceWire Project

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * ShimafujiElectricSpaceWire6PortRouter.hh
 *
 *  Created on: Mar 20, 2013
 *      Author: Takayuki Yuasa
 */

#ifndef SHIMAFUJIELECTRICSPACEWIRE6PORTROUTER_HH_
#define SHIMAFUJIELECTRICSPACEWIRE6PORTROUTER_HH_

#include <string>
#include "../RouterConfigurationPort.hh"

class ShimafujiElectricSpaceWire6PortRouter: public RouterConfigurationPort {
public:
	static const size_t NumberOfExternalPorts = 6;
	static const size_t NumberOfInternalPorts = 0;
	static const size_t MaximumPortNumber = 6;
	static constexpr double MaximumLinkFrequency = 100;

private:
	RMAPTargetNode* routerConfigurationPort;

public:
	ShimafujiElectricSpaceWire6PortRouter() {
		std::string targetNodeID = "SpaceWire6PortRouter";
		setRMAPInitiator(NULL);
		routerConfigurationPort = NULL;
		std::vector<RMAPTargetNode*> nodes = RMAPTargetNode::constructFromXMLString(getConfigurationFileAsString());
		for (size_t i = 0; i < nodes.size(); i++) {
			if (nodes[i]->getID() == targetNodeID) {
				routerConfigurationPort = nodes[i];
				return;
			}
		}
		//cannt reach here
		using namespace std;
		cerr << "TargetNodeID " << targetNodeID << " cannot be found in ShimafujiElectricSpaceWire6PortRouter." << endl;
		cerr << "Defined RMAPTargetNodes are:" << endl;
		for (size_t i = 0; i < nodes.size(); i++) {
			cerr << nodes[i]->getID() << endl;
		}
		exit(-1);
	}

public:
	virtual ~ShimafujiElectricSpaceWire6PortRouter() {

	}

public:
	void applyNewPathAddresses(std::vector<uint8_t> targetSpaceWireAddress, std::vector<uint8_t> replyAddress) {
		if (targetSpaceWireAddress.size() == 0) {
			targetSpaceWireAddress.push_back(0x00);
		} else if (targetSpaceWireAddress[targetSpaceWireAddress.size() - 1] != 0x00) {
			targetSpaceWireAddress.push_back(0x00);
		}
		routerConfigurationPort->setTargetSpaceWireAddress(targetSpaceWireAddress);
		routerConfigurationPort->setReplyAddress(replyAddress);
	}

public:
	std::vector<uint8_t> getConfigurablePorts() {
		std::vector<uint8_t> result;
		result.push_back(0x01);
		result.push_back(0x02);
		result.push_back(0x03);
		result.push_back(0x04);
		result.push_back(0x05);
		result.push_back(0x06);
		return result;
	}

public:
	virtual RMAPTargetNode* getRMAPTargetNodeInstance() {
		return routerConfigurationPort;
	}

public:
	size_t getTotalNumberOfPorts() {
		return NumberOfExternalPorts + NumberOfInternalPorts;
	}

public:
	size_t getNumberOfExternalPorts() {
		return NumberOfExternalPorts;
	}

public:
	size_t getNumberOfInternalPorts() {
		return NumberOfInternalPorts;
	}

public:
	RMAPMemoryObject* getRoutingTableMemoryObject(uint8_t logicalAddress) throw (RouterConfigurationPortException) {
		std::string id;
		std::stringstream ss;
		using namespace std;
		ss << "RoutingTable0x" << uppercase << hex << right << setw(2) << setfill('0') << (uint32_t) logicalAddress;
		id = ss.str();
		return routerConfigurationPort->findMemoryObject(id);
	}

public:
	uint32_t getRoutingTableAddress(uint8_t logicalAddress) throw (RouterConfigurationPortException) {
		return getRoutingTableMemoryObject(logicalAddress)->getAddress();
	}

private:
	std::vector<uint8_t> convertRoutingTableToPortNumbers(uint8_t* value) {
		uint32_t register32bits;
		register32bits = value[3];
		register32bits = (register32bits << 8) + value[2];
		register32bits = (register32bits << 8) + value[1];
		register32bits = (register32bits << 8) + value[0];
		uint32_t flag = 0;
		std::vector<uint8_t> result;
		for (size_t i = 0; i < 32; i++) {
			flag = 0x01 << i;
			if ((register32bits & flag) != 0) {
				result.push_back(i);
			}
		}
		return result;
	}

private:
	uint8_t* convertPortNumbersToRoutingTable(std::vector<uint8_t> portNumbers) {
		uint8_t* buffer = new uint8_t[4];
		memset(buffer, 0, 4);
		uint32_t flag = 0;
		for (size_t i = 0; i < portNumbers.size(); i++) {
			uint8_t port = portNumbers.at(i);
			if (port < 8) { //Port 0-7
				buffer[0] = (buffer[0] & ~(0x01 << port)) + (0x01 << port);
			} else { //Port 8-9
				buffer[1] = (buffer[1] & ~(0x01 << (port - 8))) + (0x01 << (port - 8));
			}
		}
		return buffer;
	}

private:
	std::vector<std::vector<uint8_t> > convertWholeRoutingTableToVector(uint8_t* buffer) {
		std::vector<std::vector<uint8_t> > result;
		size_t nLogicalAddresses = SpaceWireProtocol::MaximumLogicalAddress - SpaceWireProtocol::MinimumLogicalAddress + 1;
		for (size_t i = 0; i < nLogicalAddresses; i++) {
			result.push_back(convertRoutingTableToPortNumbers(buffer + i * 4));
		}
		return result;
	}

public:
	std::vector<uint8_t> readRoutingTable(uint8_t logicalAddress) throw (RouterConfigurationPortException,
			RMAPInitiatorException) {
		RMAPMemoryObject* routingTableForALogicalAddress = getRoutingTableMemoryObject(logicalAddress);
		throwIfRMAPInitiatorIsNULL();
		uint8_t value[4];
		try {
			rmapInitiator->read(routerConfigurationPort, routingTableForALogicalAddress->getID(), value);
		} catch (RMAPEngineException& e) {
			throw RouterConfigurationPortException(RouterConfigurationPortException::OperationFailed);
		}
		return convertRoutingTableToPortNumbers(value);
	}

public:
	std::vector<std::vector<uint8_t> > readWholeRoutingTable() throw (RouterConfigurationPortException,
			RMAPInitiatorException) {

		RMAPMemoryObject* routingTableForALogicalAddress = getRoutingTableMemoryObject(
				SpaceWireProtocol::MinimumLogicalAddress);
		size_t length = (SpaceWireProtocol::MaximumLogicalAddress - SpaceWireProtocol::MinimumLogicalAddress + 1) * 4;
		throwIfRMAPInitiatorIsNULL();
		uint8_t* buffer = new uint8_t[length];
		using namespace std;
		try {
			size_t o = 0;
			for (size_t i = SpaceWireProtocol::MinimumLogicalAddress; i < SpaceWireProtocol::MaximumLogicalAddress; i++) {
				rmapInitiator->read(routerConfigurationPort, getRoutingTableAddress(i), 4, buffer + o * 4);
				o++;
			}
		} catch (RMAPEngineException& e) {
			delete buffer;
			throw RouterConfigurationPortException(RouterConfigurationPortException::OperationFailed);
		} catch (RMAPInitiatorException& e) {
			using namespace std;
			cout << e.toString() << endl;
			delete buffer;
			throw RouterConfigurationPortException(RouterConfigurationPortException::OperationFailed);
		} catch (RMAPReplyException& e) {
			using namespace std;
			cout << e.toString() << endl;
			delete buffer;
			throw RouterConfigurationPortException(RouterConfigurationPortException::OperationFailed);
		}
		std::vector<std::vector<uint8_t> > result = convertWholeRoutingTableToVector(buffer);
		delete buffer;
		return result;
	}

public:
	void writeRoutingTable(uint8_t logicalAddress, std::vector<uint8_t> ports) throw (RouterConfigurationPortException,
			RMAPInitiatorException) {
		RMAPMemoryObject* routingTableForALogicalAddress = getRoutingTableMemoryObject(logicalAddress);
		throwIfRMAPInitiatorIsNULL();
		uint8_t* buffer = convertPortNumbersToRoutingTable(ports);
		try {
			rmapInitiator->write(routerConfigurationPort, routingTableForALogicalAddress->getID(), buffer);
			delete buffer;
		} catch (RMAPEngineException& e) {
			delete buffer;
			throw RouterConfigurationPortException(RouterConfigurationPortException::OperationFailed);
		}
	}

public:
	RMAPMemoryObject* getLinkFrequencyRegisterMemoryObject(uint8_t port) throw (RouterConfigurationPortException) {
		if (port > MaximumPortNumber) {
			throw RouterConfigurationPortException(RouterConfigurationPortException::InvalidPortNumber);
		}
		using namespace std;
		stringstream ss;
		ss << "Port" << (uint32_t) port << "ControlStatusRegister";
		return routerConfigurationPort->findMemoryObject(ss.str());
	}

public:
	uint32_t getLinkFrequencyRegisterAddress(uint8_t port) throw (RouterConfigurationPortException) {
		return getLinkFrequencyRegisterMemoryObject(port)->getAddress();
	}

public:
	std::vector<double> getAvailableLinkFrequencies(uint8_t port) {
		std::vector<double> result;
		result.push_back(100);
		result.push_back(50);
		result.push_back(25);
		result.push_back(10);
		result.push_back(12.5);
		result.push_back(5);
		result.push_back(2);
		return result;
	}

public:
	bool isSpecifiedLinkFrequencyAvailable(double linkFrequency) {
		std::vector<double> freqs = getAvailableLinkFrequencies(1);
		for (size_t i = 0; i < freqs.size(); i++) {
			if (freqs[i] == linkFrequency) {
				return true;
			}
		}
		return false;
	}

public:
	void setLinkFrequency(uint8_t port, double linkFrequency) throw (RouterConfigurationPortException,
			RMAPInitiatorException) {
		if (isLinkFrequencyValid(port, linkFrequency) == false) {
			throw RouterConfigurationPortException(RouterConfigurationPortException::InvalidLinkFrequency);
		}

		throwIfRMAPInitiatorIsNULL();

		uint8_t txdivider = 0; //100MHz
		if (linkFrequency == 100) {
			txdivider = 0;
		} else if (linkFrequency == 50) {
			txdivider = 1;
		} else if (linkFrequency == 25) {
			txdivider = 3;
		} else if (linkFrequency == 12.5) {
			txdivider = 7;
		} else if (linkFrequency == 10) {
			txdivider = 9;
		} else if (linkFrequency == 5) {
			txdivider = 19;
		} else if (linkFrequency == 2) {
			txdivider = 49;
		}

		if (rmapInitiator == NULL) {
			throw RouterConfigurationPortException(RouterConfigurationPortException::RMAPInitiatorIsNotAvailable);
		}

		RMAPMemoryObject* registerMemoryObject = getLinkFrequencyRegisterMemoryObject(port);
		uint8_t value[4];

		try {
			//29:24 = Tx Clock Divider n. TxClock = 100MHz / (n + 1)

			//first read the register
			rmapInitiator->read(routerConfigurationPort, registerMemoryObject->getID(), value);

			//change the register value
			value[3] = txdivider;

			//write back
			rmapInitiator->write(routerConfigurationPort, registerMemoryObject->getID(), value);
		} catch (RMAPInitiatorException& e) {
			throw e;
		} catch (...) {
			throw RouterConfigurationPortException(RouterConfigurationPortException::OperationFailed);
		}
	}

public:
	double getLinkFrequency(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		uint32_t txdiv = value[3] & 0x3F; /* 00111111*/
		return MaximumLinkFrequency / (txdiv + 1);
	}

private:
	void throwIfRMAPInitiatorIsNULL() throw (RouterConfigurationPortException) {
		if (rmapInitiator == NULL) {
			throw RouterConfigurationPortException(RouterConfigurationPortException::RMAPInitiatorIsNotAvailable);
		}
	}

private:
	RMAPMemoryObject* getLinkControlStatusRegisterMemoryObject(uint8_t port) throw (RouterConfigurationPortException) {
		if (port > MaximumPortNumber) {
			throw RouterConfigurationPortException(RouterConfigurationPortException::InvalidPortNumber);
		}
		using namespace std;
		stringstream ss;
		ss << "Port" << (uint32_t) port << "ControlStatusRegister";
		return
				(routerConfigurationPort->findMemoryObject(ss.str()) != NULL) ?
						routerConfigurationPort->findMemoryObject(ss.str()) :
						throw RouterConfigurationPortException(RouterConfigurationPortException::NotImplemented);
	}

public:
	void readLinkControlStatusRegister(uint8_t port, uint8_t* value) throw (RouterConfigurationPortException,
			RMAPInitiatorException) {
		RMAPMemoryObject* linkCSR = getLinkControlStatusRegisterMemoryObject(port);
		throwIfRMAPInitiatorIsNULL();
		try {
			rmapInitiator->read(routerConfigurationPort, linkCSR->getID(), value);
		} catch (RMAPEngineException& e) {
			throw RouterConfigurationPortException(RouterConfigurationPortException::OperationFailed);
		}
	}

public:
	std::string getLinkCSRAsString(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		using namespace std;
		std::stringstream ss;
		ss << "Port " << (uint32_t) port << "(0x" << hex << right << setw(2) << setfill('0') << (uint32_t) port << ") ";
		ss << setw(8) << setfill(' ') << left << (isLinkEnabled(port) ? "Enabled" : "Disabled") << " ";
		ss << "Started=" << setw(3) << setfill(' ') << left << (isLinkStarted(port) ? "yes" : "no") << " ";
		ss << "AutoStart=" << setw(3) << setfill(' ') << left << (isAutoStarted(port) ? "yes" : "no") << " ";
		ss << (isConnected(port) ? "Connected   " : "Disconnected") << " ";
		ss << setw(3) << setfill(' ') << right << getLinkFrequency(port) << " MHz(Tx)";
		return ss.str();
	}

public:
	bool isConnected(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		uint32_t connected = value[0] & 0x10; /* 00010000*/
		if (connected == 0) {
			return false;
		} else {
			return true;
		}
	}

public:
	void writeLinkControlStatusRegister(uint8_t port, uint8_t* value) throw (RouterConfigurationPortException,
			RMAPInitiatorException) {
		RMAPMemoryObject* linkCSR = getLinkControlStatusRegisterMemoryObject(port);
		throwIfRMAPInitiatorIsNULL();
		try {
			rmapInitiator->write(routerConfigurationPort, linkCSR->getID(), value);
		} catch (RMAPEngineException& e) {
			throw RouterConfigurationPortException(RouterConfigurationPortException::OperationFailed);
		}
	}

	/* [29:24] Tx Clock Divider
	 * [19]    Link Reset   (R/W) Default 0
	 * [18]    Auto Start   (R/W) Default 1
	 * [17]    Link Disable (R/W) Default 0
	 * [16]    Link Start   (R/W) Default 1
	 */

private:
	void readLinkControlFlags() {
		uint8_t value[4];
		uint8_t linkControlFlags = value[2];
		uint8_t linkStart = linkControlFlags & 0x01; //0000 0001
		uint8_t linkDisable = (linkControlFlags & 0x02) >> 1; //000 0010
		uint8_t linkEnable;
		if (linkDisable == 0) {
			linkEnable = 1;
		} else {
			linkEnable = 0;
		}
		uint8_t autoStart = (linkControlFlags & 0x04) >> 2; //000 0100
		uint8_t linkReset = (linkControlFlags & 0x08) >> 3; //000 1000
	}

public:
	void setLinkEnable(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		// linkControlFlags = value[2]
		// linkDisable = (linkControlFlags & 0x02) >> 1; //000 0010

		//set Link Disable 0
		value[2] = (value[2] & 0xFC /* 1111 1100 */) + 0x01 /* 0000 0000 */;

		writeLinkControlStatusRegister(port, value);
	}

public:
	void unsetLinkEnable(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		// linkControlFlags = value[2]
		// linkDisable = (linkControlFlags & 0x02) >> 1; //000 0010

		//set Link Disable 1
		value[2] = (value[2] & 0xF4 /* 1111 0100 */) + 0x0A /* 0000 1010 */;

		writeLinkControlStatusRegister(port, value);
	}

public:
	void setLinkDisable(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		unsetLinkEnable(port);
	}

public:
	bool isLinkEnabled(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		uint8_t linkControlFlags = value[2];
		uint8_t linkDisable = (linkControlFlags & 0x02) >> 1; //000 0010
		uint8_t linkEnable;
		if (linkDisable == 0) {
			linkEnable = 1;
		} else {
			linkEnable = 0;
		}
		if (linkEnable == 1) {
			return true;
		} else {
			return false;
		}
	}

public:
	void setLinkStart(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		// linkControlFlags = value[2]
		// linkStart = linkControlFlags & 0x01; //0000 0001

		//set Link Start 1
		value[2] = (value[2] & 0xFE /* 1111 1110 */) + 0x01 /* 0000 0001 */;

		writeLinkControlStatusRegister(port, value);
	}

public:
	void unsetLinkStart(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		// linkControlFlags = value[2]
		// linkStart = linkControlFlags & 0x01; //0000 0001

		//set Link Start 0
		value[2] = (value[2] & 0xFE /* 1111 1110 */) + 0x00 /* 0000 0000 */;

		writeLinkControlStatusRegister(port, value);
	}

public:
	bool isLinkStarted(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		uint8_t linkControlFlags = value[2];
		uint8_t linkStart = linkControlFlags & 0x01; //0000 0001
		if (linkStart == 1) {
			return true;
		} else {
			return false;
		}
	}

public:
	void setAutoStart(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		// linkControlFlags = value[2]
		// autoStart = linkControlFlags & 0x04; //0000 0100

		//set Auto Start 1
		value[2] = (value[2] & 0xFB /* 1111 1011 */) + 0x04 /* 0000 0100 */;

		writeLinkControlStatusRegister(port, value);
	}

public:
	void unsetAutoStart(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		// linkControlFlags = value[2]
		// autoStart = linkControlFlags & 0x04; //0000 0100

		//set Auto Start 0
		value[2] = (value[2] & 0xFB /* 1111 1011 */) + 0x00 /* 0000 0000 */;

		writeLinkControlStatusRegister(port, value);
	}

public:
	bool isAutoStarted(uint8_t port) throw (RouterConfigurationPortException, RMAPInitiatorException) {
		uint8_t value[4];
		readLinkControlStatusRegister(port, value);
		uint8_t linkControlFlags = value[2];
		uint8_t autoStart = (linkControlFlags & 0x04) >> 2; //000 0100
		if (autoStart == 1) {
			return true;
		} else {
			return false;
		}
	}

public:
	std::string getRegisterValueAsString(std::string memoryObjectID) throw (RouterConfigurationPortException,
			RMAPInitiatorException) {
		using namespace std;
		throwIfRMAPInitiatorIsNULL();

		stringstream ss;
		uint8_t value[4];
		rmapInitiator->read(routerConfigurationPort, memoryObjectID, value);
		uint32_t value32 = value[3] * 0x1000000 + value[2] * 0x10000 + value[1] * 0x100 + value[0];
		ss << "0x" << hex << right << setw(4) << setfill('0') << value32;
		return ss.str();
	}

public:
	std::string getFPGARevisionAsString() throw (RouterConfigurationPortException, RMAPInitiatorException) {
		return getRegisterValueAsString("DeviceID");
	}

public:
	std::string getDeviceIDAsString() throw (RouterConfigurationPortException, RMAPInitiatorException) {
		return getRegisterValueAsString("FPGA Revision");
	}

public:
	std::string getSpaceWireIPRevisionAsString() throw (RouterConfigurationPortException, RMAPInitiatorException) {
		return getRegisterValueAsString("SpaceWireIPRevision");
	}

public:
	std::string getRMAPIPRevisionAsString() throw (RouterConfigurationPortException, RMAPInitiatorException) {
		return getRegisterValueAsString("RMAPIPRevision");
	}

public:
	std::string getRevisionsAsString() throw (RouterConfigurationPortException, RMAPInitiatorException) {
		using namespace std;
		stringstream ss;
		string idDeviceID = "DeviceID";
		string idFPGARevision = "FPGA Revision";
		string idSpaceWireIPRevision = "SpaceWireIPRevision";
		string idRMAPIPRevision = "RMAPIPRevision";

		size_t width = 25;

		ss << setw(width) << setfill(' ') << right << "Device ID = " << getRegisterValueAsString(idDeviceID) << endl;
		ss << setw(width) << setfill(' ') << right << "FPGA Revision = " << getRegisterValueAsString(idFPGARevision)
				<< endl;
		ss << setw(width) << setfill(' ') << right << "SpaceWire IP Revision = "
				<< getRegisterValueAsString(idSpaceWireIPRevision) << endl;
		ss << setw(width) << setfill(' ') << right << "RMAP IP Revision = " << getRegisterValueAsString(idRMAPIPRevision);

		return ss.str();
	}

public:
	bool isPortNumberValid(uint8_t port) {
		if (port > ShimafujiElectricSpaceWire6PortRouter::MaximumPortNumber) {
			return false;
		} else {
			return true;
		}
	}

public:
	bool areAllPortNumbersValid(std::vector<uint8_t>& ports) {
		for (size_t i = 0; i < ports.size(); i++) {
			if (!isPortNumberValid(ports[i])) {
				return false;
			}
		}
		return true;
	}

public:
	void setVirtualLogicalAddress(uint8_t port, uint8_t virtualLogicalAddress) {
		std::string memoryObjectID;
		if (port == 0) {
			memoryObjectID = "Port0VirtualLogicalAddress";
		} else if (port == 1) {
			memoryObjectID = "Port1VirtualLogicalAddress";
		} else if (port == 2) {
			memoryObjectID = "Port2VirtualLogicalAddress";
		} else {
			using namespace std;
			cerr << "ShimafujiElectricSpaceWire6PortRouter::setVirtualLogicalAddress() invalid port number." << endl;
			return;
		}
		uint8_t data[4] = { virtualLogicalAddress, 0x00, 0x00, 0x00 };
		try {
			rmapInitiator->write(routerConfigurationPort, memoryObjectID, data, sizeof(data));
		} catch (RMAPInitiatorException& e) {
			using namespace std;
			cerr << "ShimafujiElectricSpaceWire6PortRouter::setVirtualLogicalAddress() exception." << endl;
			cerr << e.toString() << endl;
			exit(-1);
		}
	}

public:
	static std::string getConfigurationFileAsString() {
		std::string str = "<Configuration>"
				""
				"<RMAPTargetNode id=\"SpaceWire6PortRouter\">"
				"	<TargetLogicalAddress>0xFE</TargetLogicalAddress>"
				"	<TargetSpaceWireAddress>0x00</TargetSpaceWireAddress>"
				"	<ReplyAddress></ReplyAddress>"
				"	<Key>0x02</Key>"
				""
				"		<!-- Link Status Control Register -->"
				"	<RMAPMemoryObject id=\"Port0ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0000</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port1ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0004</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port2ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0008</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port3ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x000C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port4ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0010</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port5ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0014</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port6ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0018</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port7ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x001C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port8ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0020</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port9ControlStatusRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0024</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<!-- Routing Table -->"
				"	<RMAPMemoryObject id=\"RoutingTable0x20\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0080</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x21\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0084</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x22\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0088</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x23\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x008C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x24\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0090</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x25\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0094</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x26\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0098</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x27\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x009C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x28\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00A0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x29\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00A4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x2A\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00A8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x2B\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00AC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x2C\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00B0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x2D\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00B4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x2E\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00B8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x2F\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00BC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x30\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00C0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x31\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00C4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x32\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00C8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x33\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00CC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x34\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00D0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x35\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00D4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x36\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00D8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x37\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00DC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x38\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00E0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x39\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00E4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x3A\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00E8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x3B\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00EC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x3C\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00F0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x3D\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00F4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x3E\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00F8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x3F\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x00FC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x40\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0100</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x41\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0104</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x42\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0108</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x43\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x010C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x44\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0110</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x45\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0114</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x46\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0118</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x47\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x011C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x48\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0120</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x49\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0124</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x4A\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0128</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x4B\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x012C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x4C\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0130</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x4D\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0134</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x4E\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0138</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x4F\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x013C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x50\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0140</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x51\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0144</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x52\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0148</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x53\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x014C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x54\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0150</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x55\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0154</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x56\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0158</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x57\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x015C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x58\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0160</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x59\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0164</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x5A\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0168</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x5B\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x016C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x5C\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0170</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x5D\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0174</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x5E\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0178</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x5F\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x017C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x60\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0180</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x61\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0184</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x62\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0188</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x63\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x018C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x64\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0190</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x65\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0194</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x66\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0198</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x67\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x019C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x68\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01A0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x69\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01A4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x6A\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01A8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x6B\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01AC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x6C\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01B0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x6D\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01B4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x6E\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01B8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x6F\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01BC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x70\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01C0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x71\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01C4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x72\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01C8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x73\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01CC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x74\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01D0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x75\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01D4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x76\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01D8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x77\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01DC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x78\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01E0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x79\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01E4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x7A\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01E8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x7B\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01EC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x7C\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01F0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x7D\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01F4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x7E\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01F8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x7F\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x01FC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x80\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0200</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x81\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0204</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x82\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0208</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x83\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x020C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x84\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0210</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x85\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0214</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x86\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0218</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x87\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x021C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x88\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0220</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x89\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0224</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x8A\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0228</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x8B\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x022C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x8C\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0230</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x8D\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0234</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x8E\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0238</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x8F\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x023C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x90\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0240</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x91\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0244</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x92\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0248</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x93\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x024C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x94\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0250</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x95\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0254</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x96\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0258</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x97\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x025C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x98\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0260</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x99\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0264</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x9A\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0268</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x9B\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x026C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x9C\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0270</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x9D\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0274</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x9E\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0278</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0x9F\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x027C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA0\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0280</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA1\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0284</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA2\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0288</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA3\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x028C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA4\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0290</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA5\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0294</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA6\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0298</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA7\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x029C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA8\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02A0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xA9\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02A4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xAA\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02A8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xAB\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02AC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xAC\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02B0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xAD\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02B4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xAE\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02B8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xAF\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02BC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB0\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02C0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB1\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02C4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB2\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02C8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB3\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02CC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB4\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02D0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB5\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02D4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB6\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02D8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB7\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02DC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB8\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02E0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xB9\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02E4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xBA\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02E8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xBB\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02EC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xBC\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02F0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xBD\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02F4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xBE\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02F8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xBF\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x02FC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC0\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0300</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC1\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0304</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC2\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0308</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC3\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x030C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC4\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0310</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC5\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0314</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC6\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0318</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC7\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x031C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC8\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0320</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xC9\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0324</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xCA\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0328</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xCB\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x032C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xCC\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0330</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xCD\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0334</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xCE\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0338</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xCF\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x033C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD0\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0340</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD1\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0344</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD2\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0348</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD3\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x034C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD4\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0350</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD5\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0354</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD6\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0358</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD7\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x035C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD8\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0360</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xD9\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0364</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xDA\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0368</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xDB\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x036C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xDC\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0370</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xDD\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0374</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xDE\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0378</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xDF\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x037C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE0\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0380</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE1\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0384</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE2\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0388</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE3\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x038C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE4\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0390</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE5\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0394</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE6\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0398</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE7\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x039C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE8\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03A0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xE9\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03A4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xEA\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03A8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xEB\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03AC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xEC\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03B0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xED\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03B4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xEE\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03B8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xEF\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03BC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF0\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03C0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF1\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03C4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF2\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03C8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF3\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03CC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF4\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03D0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF5\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03D4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF6\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03D8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF7\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03DC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF8\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03E0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xF9\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03E4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xFA\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03E8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xFB\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03EC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xFC\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03F0</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xFD\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03F4</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xFE\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03F8</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RoutingTable0xFF\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x03FC</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<!-- Other registers -->"
				"	<RMAPMemoryObject id=\"ConfigRegister\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0400</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RID\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0404</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RCTRL\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0408</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"TimeCode\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0410</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"TimeCodeEnable\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x041C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"ConfigurationPortKey\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0424</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"DeviceID\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0430</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"FPGA Revision\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0434</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"SpaceWireIPRevision\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0438</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"RMAPIPRevision\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x043C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<!-- Status 2/3 -->"
				"	<RMAPMemoryObject id=\"Port0Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0440</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port1Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0444</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port2Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0448</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port3Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x044C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port4Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0450</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port5Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0454</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port6Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0458</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port7Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x045C</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port8Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0460</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				""
				"	<RMAPMemoryObject id=\"Port9Status23Register\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0464</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				//Extension only available in SpaceWire-R 3 Port Router
				//programmed on SpaceWire &-Port Router hardware
				"	<RMAPMemoryObject id=\"Port0VirtualLogicalAddress\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0470</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				"	<RMAPMemoryObject id=\"Port1VirtualLogicalAddress\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0474</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"
				"	<RMAPMemoryObject id=\"Port2VirtualLogicalAddress\">"
				"		<ExtendedAddress>0x00</ExtendedAddress>"
				"		<Address>0x0478</Address>"
				"		<Length>0x04</Length>"
				"		<Key>0x02</Key>"
				"		<AccessMode>ReadWrite</AccessMode>"
				"	</RMAPMemoryObject>"

				""
				"	</RMAPTargetNode>"
				"</Configuration>";
		return str;
	}
}
;

#endif /* SHIMAFUJIELECTRICSPACEWIRE6PORTROUTER_HH_ */
