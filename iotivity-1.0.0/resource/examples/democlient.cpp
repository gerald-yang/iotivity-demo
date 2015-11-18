//******************************************************************
//
// Copyright 2014 Intel Mobile Communications GmbH All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// OCClient.cpp : Defines the entry point for the console application.
//
#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include "OCPlatform.h"
#include "OCApi.h"

using namespace OC;
namespace PH = std::placeholders;

typedef std::map<OCResourceIdentifier, std::shared_ptr<OCResource>> DiscoveredResourceMap;

DiscoveredResourceMap discoveredResources;
std::shared_ptr<OCResource> sensorResourceA = NULL;
std::shared_ptr<OCResource> sensorResourceP = NULL;
std::shared_ptr<OCResource> ledResourceA = NULL;
std::shared_ptr<OCResource> ledResourceP = NULL;
std::shared_ptr<OCResource> lcdResourceA = NULL;
std::shared_ptr<OCResource> lcdResourceP = NULL;
std::shared_ptr<OCResource> buzzerResourceA = NULL;
std::shared_ptr<OCResource> buzzerResourceP = NULL;
std::shared_ptr<OCResource> buttonResourceA = NULL;
std::shared_ptr<OCResource> buttonResourceP = NULL;
static ObserveType observe_type = ObserveType::Observe;
std::mutex curResourceLock;

void * button_observer (void *param); 
std::mutex observe_thread_lock;

class Demo
{
public:
	std::string resource_name1 = "Arduino";
	std::string resource_name2 = "RaspberryPi2";

	int debug_mode;
	std::string influxdb_ip;

	// For Arduino Due
	double sensor_a_temp;
	int sensor_a_light;
	int sensor_a_sound;
	int led_a_status;
	std::string lcd_a_str;
	int buzzer_a;
	int button_a;
	int touch_a;
	int inObserve_a;

	// For Raspberry Pi 2
	double sensor_p_temp;
	double sensor_p_humidity;
	int sensor_p_light;
	int sensor_p_sound;
	int led_p_red;
	int led_p_green;
	int led_p_blue;
	std::string lcd_p_str;
	double buzzer_p;
	int button_p;
	int inObserve_p;

	// For server part
	OCResourceHandle sensor_a_resourceHandle;
	OCRepresentation sensor_a_rep;

	OCResourceHandle led_a_resourceHandle;
	OCRepresentation led_a_rep;

	OCResourceHandle lcd_a_resourceHandle;
	OCRepresentation lcd_a_rep;

	OCResourceHandle buzzer_a_resourceHandle;
	OCRepresentation buzzer_a_rep;

	OCResourceHandle button_a_resourceHandle;
	OCRepresentation button_a_rep;

	OCResourceHandle sensor_p_resourceHandle;
	OCRepresentation sensor_p_rep;

	OCResourceHandle led_p_resourceHandle;
	OCRepresentation led_p_rep;

	OCResourceHandle lcd_p_resourceHandle;
	OCRepresentation lcd_p_rep;

	OCResourceHandle buzzer_p_resourceHandle;
	OCRepresentation buzzer_p_rep;

	OCResourceHandle button_p_resourceHandle;
	OCRepresentation button_p_rep;

	ObservationIds m_interestedObservers;

	int observe_thread_running;


	Demo() : debug_mode(0),
		sensor_a_temp(0.0), sensor_a_light(0), sensor_a_sound(0),
		led_a_status(0),
		lcd_a_str("Arduino LCD"),
		buzzer_a(0),
		button_a(0), touch_a(0), inObserve_a(0),
		sensor_p_temp(0.0), sensor_p_humidity(0.0), sensor_p_light(0), sensor_p_sound(0),
		led_p_red(0), led_p_green(0), led_p_blue(0),
	  	lcd_p_str("LCD Demo"),
		buzzer_p(0.0),
		button_p(0), inObserve_p(0),
		observe_thread_running(0)
	{
		// Initial representation
		// Arduino
		sensor_a_rep.setUri("/gateway/sensora");
		sensor_a_rep.setValue("temperature", sensor_a_temp);
		sensor_a_rep.setValue("light", sensor_a_light);
		sensor_a_rep.setValue("sound", sensor_a_sound);

		led_p_rep.setUri("/gateway/leda");
		led_p_rep.setValue("status", led_a_status);

		led_p_rep.setUri("/gateway/lcda");
		led_p_rep.setValue("lcd", lcd_a_str);

		led_p_rep.setUri("/gateway/buzzera");
		led_p_rep.setValue("tone", buzzer_a);

		led_p_rep.setUri("/gateway/buttona");
		led_p_rep.setValue("button", button_a);
		led_p_rep.setValue("touch", touch_a);

		// Raspberry Pi 2
		sensor_p_rep.setUri("/gateway/sensorp");
		sensor_p_rep.setValue("temperature", sensor_p_temp);
		sensor_p_rep.setValue("humidity", sensor_p_humidity);
		sensor_p_rep.setValue("light", sensor_p_light);
		sensor_p_rep.setValue("sound", sensor_p_sound);

		led_p_rep.setUri("/gateway/ledp");
		led_p_rep.setValue("red", led_p_red);
		led_p_rep.setValue("green", led_p_green);
		led_p_rep.setValue("blue", led_p_blue);

		led_p_rep.setUri("/gateway/lcdp");
		led_p_rep.setValue("lcd", lcd_p_str);

		led_p_rep.setUri("/gateway/buzzerp");
		led_p_rep.setValue("buzzer", buzzer_p);

		led_p_rep.setUri("/gateway/buttonp");
		led_p_rep.setValue("button", button_p);
	}

	void createResource()
	{
		std::cout << "Create server resource" << std::endl;
		// Sensor resource
		std::string resourceURI;
		std::string resourceTypeName;
		std::string resourceInterface = DEFAULT_INTERFACE;
		OCStackResult result; 
		
		// OCResourceProperty is defined ocstack.h
		uint8_t resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;

		// Resource callback functions
		EntityHandler sensor_a_cb = std::bind(&Demo::sensor_a_entityHandler, this,PH::_1);
		EntityHandler led_a_cb = std::bind(&Demo::led_a_entityHandler, this,PH::_1);
		EntityHandler lcd_a_cb = std::bind(&Demo::lcd_a_entityHandler, this,PH::_1);
		EntityHandler buzzer_a_cb = std::bind(&Demo::buzzer_a_entityHandler, this,PH::_1);
		EntityHandler button_a_cb = std::bind(&Demo::button_a_entityHandler, this,PH::_1);

		EntityHandler sensor_p_cb = std::bind(&Demo::sensor_p_entityHandler, this,PH::_1);
		EntityHandler led_p_cb = std::bind(&Demo::led_p_entityHandler, this,PH::_1);
		EntityHandler lcd_p_cb = std::bind(&Demo::lcd_p_entityHandler, this,PH::_1);
		EntityHandler buzzer_p_cb = std::bind(&Demo::buzzer_p_entityHandler, this,PH::_1);
		EntityHandler button_p_cb = std::bind(&Demo::button_p_entityHandler, this,PH::_1);

		// This will internally create and register the resource.

		// Arduino
		// Create sensor resource
#if 0   // Don't uses sensors on Arduino
		resourceURI = "/gateway/sensora";
		resourceTypeName = "gateway.sensora";
		result = OCPlatform::registerResource(
			sensor_a_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, sensor_a_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name1
				<< " sensor resource creation was unsuccessful\n";
#endif

		// Create LED resource
		resourceURI = "/gateway/leda";
		resourceTypeName = "gateway.leda";
		result = OCPlatform::registerResource(
			led_a_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, led_a_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name1
				<< " LED resource creation was unsuccessful\n";

		// Create LCD resource
		resourceURI = "/gateway/lcda";
		resourceTypeName = "gateway.lcda";
		result = OCPlatform::registerResource(
			lcd_a_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, lcd_a_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name1
				<< " LCD resource creation was unsuccessful\n";

		// Create Buzzer resource
		resourceURI = "/gateway/buzzera";
		resourceTypeName = "gateway.buzzera";
		result = OCPlatform::registerResource(
			buzzer_a_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, buzzer_a_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name1 
				<< " buzzer resource creation was unsuccessful\n";

		// Create Button resource
		resourceURI = "/gateway/buttona";
		resourceTypeName = "gateway.buttona";
		result = OCPlatform::registerResource(
			button_a_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, button_a_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name1
				<< " button resource creation was unsuccessful\n";
	

		// Raspberry Pi 2
		// Create sensor resource
		resourceURI = "/gateway/sensorp";
		resourceTypeName = "gateway.sensorp";
		result = OCPlatform::registerResource(
			sensor_p_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, sensor_p_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name2 
				<< " sensor resource creation was unsuccessful\n";

		// Create LED resource
		resourceURI = "/gateway/ledp";
		resourceTypeName = "gateway.ledp";
		result = OCPlatform::registerResource(
			led_p_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, led_p_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name2 
				<< " LED resource creation was unsuccessful\n";

		// Create LCD resource
		resourceURI = "/gateway/lcdp";
		resourceTypeName = "gateway.lcdp";
		result = OCPlatform::registerResource(
			lcd_p_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, lcd_p_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name2 
				<< " LCD resource creation was unsuccessful\n";

		// Create Buzzer resource
		resourceURI = "/gateway/buzzerp";
		resourceTypeName = "gateway.buzzerp";
		result = OCPlatform::registerResource(
			buzzer_p_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, buzzer_p_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name2 
				<< " buzzer resource creation was unsuccessful\n";

		// Create Button resource
		resourceURI = "/gateway/buttonp";
		resourceTypeName = "gateway.buttonp";
		result = OCPlatform::registerResource(
			button_p_resourceHandle, resourceURI, resourceTypeName,
			resourceInterface, button_p_cb, resourceProperty);

		if (OC_STACK_OK != result)
			std::cout << resource_name2 
				<< " button resource creation was unsuccessful\n";
	}

	void put_led_a(OCRepresentation& rep);
	void put_lcd_a(OCRepresentation& rep);
	void put_buzzer_a(OCRepresentation& rep);

	void put_led_p(OCRepresentation& rep);
	void put_lcd_p(OCRepresentation& rep);
	void put_buzzer_p(OCRepresentation& rep);

	// gets the updated representation.
	// Arduino
	OCRepresentation get_sensor_a()
	{
		sensor_a_rep.setValue("temperature", sensor_a_temp);
		sensor_a_rep.setValue("light", sensor_a_light);
		sensor_a_rep.setValue("sound", sensor_a_sound);
		return sensor_a_rep;
	}

	OCRepresentation get_led_a()
	{
		led_a_rep.setValue("status", led_a_status);
		return led_a_rep;
	}

	OCRepresentation get_lcd_a()
	{
		lcd_a_rep.setValue("lcd", lcd_a_str);
		return lcd_a_rep;
	}

	OCRepresentation get_buzzer_a()
	{
		buzzer_a_rep.setValue("tone", buzzer_a);
		return buzzer_a_rep;
	}

	OCRepresentation get_button_a()
	{
		button_a_rep.setValue("button", button_a);
		button_a_rep.setValue("touch", touch_a);
		return button_a_rep;
	}

	// Raspberry Pi 2
	OCRepresentation get_sensor_p()
	{
		sensor_p_rep.setValue("temperature", sensor_p_temp);
		sensor_p_rep.setValue("humidity", sensor_p_humidity);
		sensor_p_rep.setValue("light", sensor_p_light);
		sensor_p_rep.setValue("sound", sensor_p_sound);
		return sensor_p_rep;
	}

	OCRepresentation get_led_p()
	{
		led_p_rep.setValue("red", led_p_red);
		led_p_rep.setValue("green", led_p_green);
		led_p_rep.setValue("blue", led_p_blue);
		return led_p_rep;
	}

	OCRepresentation get_lcd_p()
	{
		lcd_p_rep.setValue("lcd", lcd_p_str);
		return lcd_p_rep;
	}

	OCRepresentation get_buzzer_p()
	{
		buzzer_p_rep.setValue("buzzer", buzzer_p);
		return buzzer_p_rep;
	}

	OCRepresentation get_button_p()
	{
		button_p_rep.setValue("button", button_p);
		return button_p_rep;
	}

private:
	// This is just a sample implementation of entity handler.
	// Entity handler can be implemented in several ways by the manufacturer
	OCEntityHandlerResult sensor_a_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name1 << " In Server sensor entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;
		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//std::cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty())
					std::cout << "\nQuery processing upto entityHandler" << std::endl;

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET")
				{
					std::cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_sensor_a());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
						ehResult = OC_EH_OK;
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
					std::cout << resource_name1 << 
						" sensors don't have PUT method" << std::endl;
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
					std::cout << resource_name1 << 
						" sensors don't have POST method" << std::endl;
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();
				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult led_a_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name1 << " In Server LED entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					std::cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_led_a());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
					OCRepresentation rep = request->getResourceRepresentation();

					// Do related operations related to PUT request
					// Update the lightResource
					put_led_a(rep);
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_led_a());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult lcd_a_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name1 << " In Server LCD entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					std::cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_lcd_a());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
					OCRepresentation rep = request->getResourceRepresentation();

					put_lcd_a(rep);
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_lcd_a());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult buzzer_a_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name1 << " In Server buzzer entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					std::cout << "\trequestType : GET\n";
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
					OCRepresentation rep = request->getResourceRepresentation();

					put_buzzer_a(rep);
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_buzzer_a());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult button_a_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name1 << " In Server button entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//std::cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					std::cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_button_a());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					std::cout << "Register observer" << std::endl;
					m_interestedObservers.push_back(observationInfo.obsId);
					inObserve_a = 1;
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					std::cout << "Un-register observer" << std::endl;
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
					inObserve_a = 0;
				}

            			pthread_t threadId;

				observe_thread_lock.lock();
            			if(!observe_thread_running) {
					pthread_create (&threadId, NULL, button_observer, (void *)this);
					observe_thread_running = 1;
				}
				observe_thread_lock.unlock();
				std::cout << "\trequestFlag : Observer\n";
				ehResult = OC_EH_OK;
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}


	// Raspberry Pi 2
	OCEntityHandlerResult sensor_p_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name2 << " In Server sensor entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;
		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//std::cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty())
					std::cout << "\nQuery processing upto entityHandler" << std::endl;

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET")
				{
					std::cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_sensor_p());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
						ehResult = OC_EH_OK;
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
					std::cout << resource_name2 << 
						" sensors don't have PUT method" << std::endl;
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
					std::cout << resource_name2 << 
						" sensors don't have POST method" << std::endl;
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();
				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult led_p_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name2 << " In Server LED entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					std::cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_led_p());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
					OCRepresentation rep = request->getResourceRepresentation();

					// Do related operations related to PUT request
					// Update the lightResource
					put_led_p(rep);
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_led_p());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult lcd_p_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name2 << " In Server LCD entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					std::cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_lcd_p());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
					OCRepresentation rep = request->getResourceRepresentation();

					put_lcd_p(rep);
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_lcd_p());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult buzzer_p_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name2 << " In Server buzzer entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					std::cout << "\trequestType : GET\n";
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
					OCRepresentation rep = request->getResourceRepresentation();

					put_buzzer_p(rep);
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_buzzer_p());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					m_interestedObservers.push_back(observationInfo.obsId);
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
				}
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

	OCEntityHandlerResult button_p_entityHandler(std::shared_ptr<OCResourceRequest> request)
	{
		std::cout << resource_name2 << " In Server button entity handler:\n";
		OCEntityHandlerResult ehResult = OC_EH_ERROR;

		if(request) {
			// Get the request type and request flag
			std::string requestType = request->getRequestType();
			int requestFlag = request->getRequestHandlerFlag();

			if(requestFlag & RequestHandlerFlag::RequestFlag) {
				//std::cout << "\t\trequestFlag : Request\n";
				auto pResponse = std::make_shared<OC::OCResourceResponse>();
				pResponse->setRequestHandle(request->getRequestHandle());
				pResponse->setResourceHandle(request->getResourceHandle());

				// Check for query params (if any)
				QueryParamsMap queries = request->getQueryParameters();

				if (!queries.empty()) {
					std::cout << "\nQuery processing upto entityHandler" << std::endl;
				}

				for (auto it : queries) {
					std::cout << "Query key: " << it.first << 
						" value : " << it.second << std:: endl;
				}

				if(requestType == "GET") {
					std::cout << "\trequestType : GET\n";
					pResponse->setErrorCode(200);
					pResponse->setResponseResult(OC_EH_OK);
					pResponse->setResourceRepresentation(get_button_p());
					if(OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
						ehResult = OC_EH_OK;
					}
				} else if(requestType == "PUT") {
					std::cout << "\trequestType : PUT\n";
				} else if(requestType == "POST") {
					std::cout << "\trequestType : POST\n";
				} else if(requestType == "DELETE") {
					std::cout << "\tDelete request received" << std::endl;
				}
			}

			if(requestFlag & RequestHandlerFlag::ObserverFlag) {
				ObservationInfo observationInfo = request->getObservationInfo();

				if(ObserveAction::ObserveRegister == observationInfo.action) {
					std::cout << "Register observer" << std::endl;
					m_interestedObservers.push_back(observationInfo.obsId);
					inObserve_p = 1;
				} else if(ObserveAction::ObserveUnregister == observationInfo.action) {
					std::cout << "Un-register observer" << std::endl;
					m_interestedObservers.erase(std::remove(
							m_interestedObservers.begin(),
							m_interestedObservers.end(),
							observationInfo.obsId),
							m_interestedObservers.end());
					inObserve_p = 0;
				}

            			pthread_t threadId;

				observe_thread_lock.lock();
            			if(!observe_thread_running) {
					pthread_create (&threadId, NULL, button_observer, (void *)this);
					observe_thread_running = 1;
				}
				observe_thread_lock.unlock();
				std::cout << "\trequestFlag : Observer\n";
				ehResult = OC_EH_OK;
			}
		} else {
			std::cout << "Request invalid" << std::endl;
		}

		return ehResult;
	}

};

Demo mydemo;

void * button_observer (void *param) {
	Demo *demo = (Demo *)param;

	int button_p_old = 0;
	int button_a_old = 0;
	int touch_a_old = 0;

	while(1) {
		sleep(1);
		if (demo->inObserve_a) {
			if(button_a_old != demo->button_a) {
				std::cout << "Notifying observers with resource handle: " << demo->button_a_resourceHandle << std::endl;

				OCStackResult result = OC_STACK_OK;

				result = OCPlatform::notifyAllObservers(demo->button_a_resourceHandle);

				if(OC_STACK_NO_OBSERVERS == result) {
					std::cout << "No More observers, stopping notifications" << std::endl;
					demo->inObserve_a = 0;
				}

				button_a_old = demo->button_a;
			}

			if(touch_a_old != demo->touch_a) {
				std::cout << "Notifying observers with resource handle: " << demo->button_a_resourceHandle << std::endl;

				OCStackResult result = OC_STACK_OK;

				result = OCPlatform::notifyAllObservers(demo->button_a_resourceHandle);

				if(OC_STACK_NO_OBSERVERS == result) {
					std::cout << "No More observers, stopping notifications" << std::endl;
					demo->inObserve_a = 0;
				}

				touch_a_old = demo->touch_a;
			}
		}

		if (demo->inObserve_p) {
			if(button_p_old != demo->button_p) {
				std::cout << "Notifying observers with resource handle: " << demo->button_p_resourceHandle << std::endl;

				OCStackResult result = OC_STACK_OK;

				result = OCPlatform::notifyAllObservers(demo->button_p_resourceHandle);

				if(OC_STACK_NO_OBSERVERS == result) {
					std::cout << "No More observers, stopping notifications" << std::endl;
					demo->inObserve_p = 0;
				}

				button_p_old = demo->button_p;
			}

		}
	}

	return NULL;
}

void sensor_write_db()
{
	std::string db_cmd;
	std::string url;
       
	url = "curl -i -XPOST 'http://";
	url += mydemo.influxdb_ip;
	url += "/write?db=fukuoka' --data-binary ";

	// Temperature sensor
	db_cmd = url;
	db_cmd += "'temperature,sensor=1 value=";
       	db_cmd += std::to_string(mydemo.sensor_p_temp);
	db_cmd += "'";
	std::cout << db_cmd.c_str() << std::endl;
	if(system(db_cmd.c_str()) == -1) {
		std::cout << "System command failed:" << std::endl;
		std::cout << db_cmd << std::endl;
	}

	// Humidity sensor
	db_cmd = url;
	db_cmd += "'humidity,sensor=1 value=";
       	db_cmd += std::to_string(mydemo.sensor_p_humidity);
	db_cmd += "'";
	std::cout << db_cmd.c_str() << std::endl;
	if(system(db_cmd.c_str()) == -1) {
		std::cout << "System command failed:" << std::endl;
		std::cout << db_cmd << std::endl;
	}

	// Light sensor
	db_cmd = url;
	db_cmd += "'light,sensor=1 value=";
       	db_cmd += std::to_string(mydemo.sensor_p_light);
	db_cmd += "'";
	std::cout << db_cmd.c_str() << std::endl;
	if(system(db_cmd.c_str()) == -1) {
		std::cout << "System command failed:" << std::endl;
		std::cout << db_cmd << std::endl;
	}

	// Sound sensor
	db_cmd = url;
	db_cmd += "'sound,sensor=1 value=";
       	db_cmd += std::to_string(mydemo.sensor_p_sound);
	db_cmd += "'";
	std::cout << db_cmd.c_str() << std::endl;
	if(system(db_cmd.c_str()) == -1) {
		std::cout << "System command failed:" << std::endl;
		std::cout << db_cmd << std::endl;
	}
}

void onObserveButtonP(const HeaderOptions /*headerOptions*/, const OCRepresentation& rep,
                    const int& eCode, const int& sequenceNumber)
{
	std::cout << "onObserveButton" << std::endl;
	try {
		if(eCode == OC_STACK_OK && sequenceNumber != OC_OBSERVE_NO_OPTION) {
			if(sequenceNumber == OC_OBSERVE_REGISTER)
				std::cout << "Observe registration action is successful" << std::endl;
			else if(sequenceNumber == OC_OBSERVE_DEREGISTER)
				std::cout << "Observe De-registration action is successful" << std::endl;

			std::cout << "OBSERVE RESULT:"<<std::endl;
			std::cout << "\tSequenceNumber: "<< sequenceNumber << std::endl;
			rep.getValue("button", mydemo.button_p);
			std::cout << "button: " << mydemo.button_p << std::endl;
		} else {
			if(sequenceNumber == OC_OBSERVE_NO_OPTION) {
				std::cout << "Observe registration or de-registration action is failed" 
					<< std::endl;
			} else {
				std::cout << "onObserveButton Response error: " << eCode << std::endl;
				std::exit(-1);
			}
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onObserveButton" << std::endl;
	}
}

void onObserveButtonA(const HeaderOptions /*headerOptions*/, const OCRepresentation& rep,
                    const int& eCode, const int& sequenceNumber)
{
	std::cout << "onObserveButtonA" << std::endl;
	try {
		if(eCode == OC_STACK_OK && sequenceNumber != OC_OBSERVE_NO_OPTION) {
			if(sequenceNumber == OC_OBSERVE_REGISTER)
				std::cout << "Observe registration action is successful (Arduino)" << std::endl;
			else if(sequenceNumber == OC_OBSERVE_DEREGISTER)
				std::cout << "Observe De-registration action is successful (Arduino)" << std::endl;

			std::cout << "OBSERVE RESULT:"<<std::endl;
			std::cout << "\tSequenceNumber: "<< sequenceNumber << std::endl;
			rep.getValue("button", mydemo.button_a);
			rep.getValue("touch", mydemo.touch_a);
			std::cout << "button: " << mydemo.button_a << std::endl;
			std::cout << "touch: " << mydemo.touch_a << std::endl;
		} else {
			if(sequenceNumber == OC_OBSERVE_NO_OPTION) {
				std::cout << "Observe registration or de-registration action is failed (Arduino)" 
					<< std::endl;
			} else {
				std::cout << "onObserveButton Response error: " << eCode << std::endl;
				std::exit(-1);
			}
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onObserveButton" << std::endl;
	}
}

#if 0
void onPost(const HeaderOptions& /*headerOptions*/,
        const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK || eCode == OC_STACK_RESOURCE_CREATED) {
			std::cout << "POST request was successful" << std::endl;

			if(rep.hasAttribute("createduri")) {
				std::cout << "\tUri of the created resource: "
					<< rep.getValue<std::string>("createduri") << std::endl;
			} else {
				rep.getValue("temperature", mydemo.m_temp);
				rep.getValue("humidity", mydemo.m_humidity);
				rep.getValue("name", mydemo.m_name);

				std::cout << "\ttemperature: " << mydemo.m_temp << std::endl;
				std::cout << "\thumidity: " << mydemo.m_humidity << std::endl;
				std::cout << "\tname: " << mydemo.m_name << std::endl;
			}

			OCRepresentation rep2;

			std::cout << "Posting light representation..."<<std::endl;

			mydemo.m_temp = 1; 
			mydemo.m_humidity = 2;

			rep2.setValue("temperature", mydemo.m_temp);
			rep2.setValue("humidity", mydemo.m_humidity);
			curResource->post(rep2, QueryParamsMap(), &onPost2);
		} else {
			std::cout << "onPost Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onPost" << std::endl;
	}
}

// Local function to put a different state for this resource
void postDemoRepresentation(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		OCRepresentation rep;

		std::cout << "Posting light representation..."<<std::endl;

		mydemo.m_temp = 5;
		mydemo.m_humidity = 6;

		rep.setValue("temperature", mydemo.m_temp);
		rep.setValue("humidity", mydemo.m_humidity);
		// Invoke resource's post API with rep, query map and the callback parameter
		resource->post(rep, QueryParamsMap(), &onPost);
	}
}
#endif

// callback handler on PUT request
void onPutSensor(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "Sensor PUT request was successful" << std::endl;

			rep.getValue("temperature", mydemo.sensor_p_temp);
			rep.getValue("humidity", mydemo.sensor_p_humidity);
			rep.getValue("light", mydemo.sensor_p_light);
			rep.getValue("sound", mydemo.sensor_p_sound);

			std::cout << "\ttemperature: " << mydemo.sensor_p_humidity << std::endl;
			std::cout << "\thumidity: " << mydemo.sensor_p_humidity << std::endl;
			std::cout << "\tlight: " << mydemo.sensor_p_light << std::endl;
			std::cout << "\tsound: " << mydemo.sensor_p_sound << std::endl;

		} else {
			std::cout << "onPutSensor Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onPutSensor" << std::endl;
	}
}

void onPutLed(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "Led PUT request was successful" << std::endl;

			rep.getValue("red", mydemo.led_p_red);
			rep.getValue("green", mydemo.led_p_green);
			rep.getValue("blue", mydemo.led_p_blue);

			std::cout << "\tRed LED: " << mydemo.led_p_red << std::endl;
			std::cout << "\tGreen LED: " << mydemo.led_p_green << std::endl;
			std::cout << "\tBlue LED: " << mydemo.led_p_blue << std::endl;
		} else {
			std::cout << "onPutLed Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onPutLed" << std::endl;
	}
}

void onPutLedA(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "Arduino Led PUT request was successful" << std::endl;

			rep.getValue("status", mydemo.led_a_status);

			std::cout << "\tArduino LED status: " << mydemo.led_a_status << std::endl;
		} else {
			std::cout << "onPutLed Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onPutLed" << std::endl;
	}
}

void onPutLcd(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "LCD PUT request was successful" << std::endl;

			rep.getValue("lcd", mydemo.lcd_p_str);

			std::cout << "\tLCD string: " << mydemo.lcd_p_str << std::endl;
		} else {
			std::cout << "onPutLcd Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onPutLcd" << std::endl;
	}
}

void onPutLcdA(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "Arduino LCD PUT request was successful" << std::endl;

			rep.getValue("lcd", mydemo.lcd_a_str);

			std::cout << "\tLCD string: " << mydemo.lcd_a_str << std::endl;
		} else {
			std::cout << "onPutLcdA Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onPutLcdA" << std::endl;
	}
}

void onPutBuzzer(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "Buzzer PUT request was successful" << std::endl;

			rep.getValue("buzzer", mydemo.buzzer_p);
		} else {
			std::cout << "onPutBuzzer Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onPutBuzzer" << std::endl;
	}
}

void onPutBuzzerA(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "Arduino Buzzer PUT request was successful" << std::endl;

			rep.getValue("tone", mydemo.buzzer_a);
		} else {
			std::cout << "onPutBuzzerA Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onPutBuzzerA" << std::endl;
	}
}

// Local function to put a different state for this resource
void putLedRepresentationP(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		OCRepresentation rep;

		std::cout << "Putting LED representation..."<<std::endl;

		rep.setValue("red", mydemo.led_p_red);
		rep.setValue("green", mydemo.led_p_green);
		rep.setValue("blue", mydemo.led_p_blue);

		// Invoke resource's put API with rep, query map and the callback parameter
		resource->put(rep, QueryParamsMap(), &onPutLed);
	}
}

void putLedRepresentationA(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		OCRepresentation rep;

		std::cout << "Putting Arduno LED representation..."<<std::endl;

		rep.setValue("status", mydemo.led_a_status);

		// Invoke resource's put API with rep, query map and the callback parameter
		resource->put(rep, QueryParamsMap(), &onPutLedA);
	}
}

void putLcdRepresentationP(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		OCRepresentation rep;

		std::cout << "Putting LCD representation..."<<std::endl;

		rep.setValue("lcd", mydemo.lcd_p_str);

		// Invoke resource's put API with rep, query map and the callback parameter
		resource->put(rep, QueryParamsMap(), &onPutLcd);
	}
}

void putLcdRepresentationA(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		OCRepresentation rep;

		std::cout << "Arduino Putting LCD representation..."<<std::endl;

		rep.setValue("lcd", mydemo.lcd_a_str);

		// Invoke resource's put API with rep, query map and the callback parameter
		resource->put(rep, QueryParamsMap(), &onPutLcdA);
	}
}

void putBuzzerRepresentationP(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		OCRepresentation rep;

		std::cout << "Putting Buzzer representation..."<<std::endl;

		rep.setValue("buzzer", mydemo.buzzer_p);

		// Invoke resource's put API with rep, query map and the callback parameter
		resource->put(rep, QueryParamsMap(), &onPutBuzzer);
	}
}

void putBuzzerRepresentationA(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		OCRepresentation rep;

		std::cout << "Arduino Putting Buzzer representation..."<<std::endl;

		rep.setValue("tone", mydemo.buzzer_a);

		// Invoke resource's put API with rep, query map and the callback parameter
		resource->put(rep, QueryParamsMap(), &onPutBuzzerA);
	}
}

// put methods
void Demo::put_led_a(OCRepresentation& rep)
{
	rep.getValue("status", mydemo.led_a_status);
	putLedRepresentationA(ledResourceA);
}

void Demo::put_lcd_a(OCRepresentation& rep)
{
	rep.getValue("lcd", mydemo.lcd_a_str);
	putLcdRepresentationA(lcdResourceA);
}

void Demo::put_buzzer_a(OCRepresentation& rep)
{
	rep.getValue("tone", mydemo.buzzer_a);
	std::cout << "tone: " << mydemo.buzzer_a << std::endl;
	putBuzzerRepresentationA(buzzerResourceA);
}

void Demo::put_led_p(OCRepresentation& rep)
{
	rep.getValue("red", mydemo.led_p_red);
	rep.getValue("green", mydemo.led_p_green);
	rep.getValue("blue", mydemo.led_p_blue);
	putLedRepresentationP(ledResourceP);
}

void Demo::put_lcd_p(OCRepresentation& rep)
{
	rep.getValue("lcd", mydemo.lcd_p_str);
	putLcdRepresentationP(lcdResourceP);
}

void Demo::put_buzzer_p(OCRepresentation& rep)
{
	rep.getValue("buzzer", mydemo.buzzer_p);
	putBuzzerRepresentationP(buzzerResourceP);
}

// Callback handler on GET request
void onGetSensorP(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "GET request was successful" << std::endl;
			std::cout << "Resource URI: " << rep.getUri() << std::endl;

			rep.getValue("temperature", mydemo.sensor_p_temp);
			rep.getValue("humidity", mydemo.sensor_p_humidity);
			rep.getValue("light", mydemo.sensor_p_light);
			rep.getValue("sound", mydemo.sensor_p_sound);

			std::cout << "\ttemperature: " << mydemo.sensor_p_temp << std::endl;
			std::cout << "\thumidity: " << mydemo.sensor_p_humidity << std::endl;
			std::cout << "\tlight: " << mydemo.sensor_p_light << std::endl;
			std::cout << "\tsound: " << mydemo.sensor_p_sound << std::endl;

			sensor_write_db();
		} else {
			std::cout << "onGET Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onGetSensor" << std::endl;
	}
}

void onGetSensorA(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "GET request was successful" << std::endl;
			std::cout << "Resource URI: " << rep.getUri() << std::endl;

			rep.getValue("temperature", mydemo.sensor_a_temp);
			rep.getValue("light", mydemo.sensor_a_light);
			rep.getValue("sound", mydemo.sensor_a_sound);

			std::cout << "\ttemperature: " << mydemo.sensor_a_temp << std::endl;
			std::cout << "\tlight: " << mydemo.sensor_a_light << std::endl;
			std::cout << "\tsound: " << mydemo.sensor_a_sound << std::endl;

			//sensor_write_db();
		} else {
			std::cout << "onGET Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onGetSensorA" << std::endl;
	}
}

void onGetLedP(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "GET request was successful" << std::endl;
			std::cout << "Resource URI: " << rep.getUri() << std::endl;

			rep.getValue("red", mydemo.led_p_red);
			rep.getValue("green", mydemo.led_p_green);
			rep.getValue("blue", mydemo.led_p_blue);

			std::cout << "\tred: " << mydemo.led_p_red << std::endl;
			std::cout << "\tgreen: " << mydemo.led_p_green << std::endl;
			std::cout << "\tblue: " << mydemo.led_p_blue << std::endl;
		} else {
			std::cout << "onGET Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onGetLed" << std::endl;
	}
}

void onGetLedA(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "GET request was successful" << std::endl;
			std::cout << "Resource URI: " << rep.getUri() << std::endl;

			rep.getValue("status", mydemo.led_a_status);

			std::cout << "\tLED status: " << mydemo.led_a_status << std::endl;
		} else {
			std::cout << "onGET Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onGetLedA" << std::endl;
	}
}

void onGetLcdP(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "GET request was successful" << std::endl;
			std::cout << "Resource URI: " << rep.getUri() << std::endl;

			rep.getValue("lcd", mydemo.lcd_p_str);

			std::cout << "\tLED status: " << mydemo.lcd_p_str << std::endl;
		} else {
			std::cout << "onGET Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onGetLedA" << std::endl;
	}
}

void onGetLcdA(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "GET request was successful" << std::endl;
			std::cout << "Resource URI: " << rep.getUri() << std::endl;

			rep.getValue("lcd", mydemo.lcd_a_str);

			std::cout << "\tLED status: " << mydemo.lcd_a_str << std::endl;
		} else {
			std::cout << "onGET Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onGetLedA" << std::endl;
	}
}

void onGetButton(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "GET request was successful" << std::endl;
			std::cout << "Resource URI: " << rep.getUri() << std::endl;

			rep.getValue("button", mydemo.button_p);

			std::cout << "\tbutton: " << mydemo.button_p << std::endl;
		} else {
			std::cout << "onGET Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onGetButton" << std::endl;
	}
}

void onGetButtonA(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
	try {
		if(eCode == OC_STACK_OK) {
			std::cout << "GET request was successful" << std::endl;
			std::cout << "Resource URI: " << rep.getUri() << std::endl;

			rep.getValue("button", mydemo.button_a);
			rep.getValue("touch", mydemo.touch_a);

			std::cout << "\tbutton: " << mydemo.button_a << std::endl;
			std::cout << "\ttouch: " << mydemo.touch_a << std::endl;
		} else {
			std::cout << "onGET Response error: " << eCode << std::endl;
			std::exit(-1);
		}
	}
	catch(std::exception& e) {
		std::cout << "Exception: " << e.what() << " in onGetButtonA" << std::endl;
	}
}

			
// Local function to get representation of light resource
void getSensorRepresentationP(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		std::cout << "Getting Sensor Representation..."<<std::endl;

		// Invoke resource's get API with the callback parameter
		QueryParamsMap test;
		resource->get(test, &onGetSensorP);
	}
}

void getSensorRepresentationA(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		std::cout << "Getting Sensor A Representation..."<<std::endl;

		// Invoke resource's get API with the callback parameter
		QueryParamsMap test;
		resource->get(test, &onGetSensorA);
	}
}

void getLedRepresentationP(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		std::cout << "Getting LED Representation..."<<std::endl;

		// Invoke resource's get API with the callback parameter
		QueryParamsMap test;
		resource->get(test, &onGetLedP);
	}
}

void getLedRepresentationA(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		std::cout << "Getting Arduino LED Representation..."<<std::endl;

		// Invoke resource's get API with the callback parameter
		QueryParamsMap test;
		resource->get(test, &onGetLedA);
	}
}

void getLcdRepresentationP(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		std::cout << "Getting LCD Representation..."<<std::endl;

		// Invoke resource's get API with the callback parameter
		QueryParamsMap test;
		resource->get(test, &onGetLcdP);
	}
}

void getLcdRepresentationA(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		std::cout << "Getting LCD Representation..."<<std::endl;

		// Invoke resource's get API with the callback parameter
		QueryParamsMap test;
		resource->get(test, &onGetLcdA);
	}
}

void getButtonRepresentationP(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		std::cout << "Getting Button Representation..."<<std::endl;

		// Invoke resource's get API with the callback parameter
		QueryParamsMap test;
		resource->get(test, &onGetButton);
	}
}

void getButtonRepresentationA(std::shared_ptr<OCResource> resource)
{
	if(resource) {
		std::cout << "Getting Arduino Button Representation..."<<std::endl;

		// Invoke resource's get API with the callback parameter
		QueryParamsMap test;
		resource->get(test, &onGetButtonA);
	}
}

// Callback to found resources
void foundResource(std::shared_ptr<OCResource> resource)
{
	std::cout << "In foundResource\n";
	std::string resourceURI;
	std::string hostAddress;

	try {
		std::lock_guard<std::mutex> lock(curResourceLock);

		if(discoveredResources.find(resource->uniqueIdentifier()) == discoveredResources.end()) {
			std::cout << "Found resource " << resource->uniqueIdentifier() <<
				" for the first time on server with ID: "<< resource->sid()<<std::endl;

			discoveredResources[resource->uniqueIdentifier()] = resource;
		} else {
			std::cout<<"Found resource "<< resource->uniqueIdentifier() << " again!"<<std::endl;
		}

		// Do some operations with resource object.
		if(resource) {
			std::cout<<"DISCOVERED Resource:"<<std::endl;
			// Get the resource URI
			resourceURI = resource->uri();
			std::cout << "\tURI of the resource: " << resourceURI << std::endl;

			// Get the resource host address
			hostAddress = resource->host();
			std::cout << "\tHost address of the resource: " << hostAddress << std::endl;

			// Get the resource types
			std::cout << "\tList of resource types: " << std::endl;
			for(auto &resourceTypes : resource->getResourceTypes()) {
				std::cout << "\t\t" << resourceTypes << std::endl;
			}

			// Get the resource interfaces
			std::cout << "\tList of resource interfaces: " << std::endl;
			for(auto &resourceInterfaces : resource->getResourceInterfaces()) {
				std::cout << "\t\t" << resourceInterfaces << std::endl;
			}

			if(resourceURI == "/grovepi/sensor") {
				if(sensorResourceP) {
					std::cout << "Found another sensor resource, ignoring" << std::endl;
				} else {
					std::cout << "Find sensor resource" << std::endl;
					sensorResourceP = resource;
				}
			}

			if(resourceURI == "/grove/sensor") {
				if(sensorResourceA) {
					std::cout << "Found another arduino sensor resource, ignoring" << std::endl;
				} else {
					std::cout << "Find Arduino sensor resource" << std::endl;
					sensorResourceA = resource;
				}
			}

			if(resourceURI == "/grovepi/led") {
				if(ledResourceP) {
					std::cout << "Found another LED resource, ignoring" << std::endl;
				} else {
					std::cout << "Find LED resource" << std::endl;
					ledResourceP = resource;
					getLedRepresentationP(ledResourceP);
				}
			}

			if(resourceURI == "/grove/led") {
				if(ledResourceA) {
					std::cout << "Found another arduino led resource, ignoring" << std::endl;
				} else {
					std::cout << "Find Arduino led resource" << std::endl;
					ledResourceA = resource;
					getLedRepresentationA(ledResourceA);
				}
			}

			if(resourceURI == "/grovepi/lcd") {
				if(lcdResourceP) {
					std::cout << "Found another LCD resource, ignoring" << std::endl;
				} else {
					std::cout << "Find LCD resource" << std::endl;
					lcdResourceP = resource;
					getLcdRepresentationP(lcdResourceP);
				}
			}

			if(resourceURI == "/grove/lcd") {
				if(lcdResourceA) {
					std::cout << "Found another Arduino LCD resource, ignoring" << std::endl;
				} else {
					std::cout << "Find Arduino LCD resource" << std::endl;
					lcdResourceA = resource;
					getLcdRepresentationA(lcdResourceA);
				}
			}

			if(resourceURI == "/grovepi/buzzer") {
				if(buzzerResourceP) {
					std::cout << "Found another buzzer resource, ignoring" << std::endl;
				} else {
					std::cout << "Find buzzer resource" << std::endl;
					buzzerResourceP = resource;
				}
			}

			if(resourceURI == "/grove/buzzer") {
				if(buzzerResourceA) {
					std::cout << "Found another Arduino buzzer resource, ignoring" << std::endl;
				} else {
					std::cout << "Find Arduino buzzer resource" << std::endl;
					buzzerResourceA = resource;
				}
			}

			if(resourceURI == "/grovepi/button") {
				if(buttonResourceP) {
					std::cout << "Found another button resource, ignoring" << std::endl;
				} else {
					std::cout << "Find button resource" << std::endl;
					buttonResourceP = resource;
					buttonResourceP->observe(observe_type, QueryParamsMap(), &onObserveButtonP);
				}
			}

			if(resourceURI == "/grove/button") {
				if(buttonResourceA) {
					std::cout << "Found another Arduino button resource, ignoring" << std::endl;
				} else {
					std::cout << "Find Arduino button resource" << std::endl;
					buttonResourceA = resource;
					buttonResourceA->observe(observe_type, QueryParamsMap(), &onObserveButtonA);
				}
			}

		} else {
			// Resource is invalid
			std::cout << "Resource is invalid" << std::endl;
		}
	}
	catch(std::exception& e) {
		std::cerr << "Exception in foundResource: "<< e.what() << std::endl;
	}
}

void printUsage()
{
	std::cout << std::endl;
	std::cout << "---------------------------------------------------------------------\n";
	std::cout << "Usage :" << std::endl;
	std::cout << "democlient <influxdb IP address>:<port>" << std::endl;
	std::cout << "---------------------------------------------------------------------\n\n";
}

static FILE* client_open(const char* /*path*/, const char *mode)
{
	return fopen("./oic_svr_db_client.json", mode);
}

static void sensor_p_read()
{
	getSensorRepresentationP(sensorResourceP);
}

static void sensor_a_read()
{
	getSensorRepresentationA(sensorResourceA);
}

static void led_p_read()
{
	getLedRepresentationP(ledResourceP);
}

static void led_p_write(int led)
{
	switch(led) {
		case 1:
			mydemo.led_p_red = 1;
			break;
		case 2:
			mydemo.led_p_green = 1;
			break;
		case 3:
			mydemo.led_p_blue = 1;
			break;
		case 4:
			mydemo.led_p_red = 0;
			break;
		case 5:
			mydemo.led_p_green = 0;
			break;
		case 6:
			mydemo.led_p_blue = 0;
			break;
	}

	putLedRepresentationP(ledResourceP);
}
static void led_a_write(int status)
{
	mydemo.led_a_status = status;

	putLedRepresentationA(ledResourceA);
}

static void lcd_p_write(std::string str)
{
	mydemo.lcd_p_str = str;
	putLcdRepresentationP(lcdResourceP);
}

static void lcd_a_write(std::string str)
{
	mydemo.lcd_a_str = str;
	putLcdRepresentationA(lcdResourceA);
}

static void buzzer_p_write(double b)
{
	mydemo.buzzer_p = b;
	putBuzzerRepresentationP(buzzerResourceP);
}

static void buzzer_a_write(int tone)
{
	mydemo.buzzer_a = tone;
	putBuzzerRepresentationA(buzzerResourceA);
}

static void button_p_read()
{
	getButtonRepresentationP(buttonResourceP);
}

static void button_p_register(int cmd)
{
	if(cmd == 1)
		buttonResourceP->observe(observe_type, QueryParamsMap(), &onObserveButtonP);
	else if(cmd == 2)
		buttonResourceP->cancelObserve();
	else
		button_p_read();
}

static void button_a_read()
{
	getButtonRepresentationA(buttonResourceA);
}

static void button_a_register(int cmd)
{
	if(cmd == 1)
		buttonResourceA->observe(observe_type, QueryParamsMap(), &onObserveButtonA);
	else if(cmd == 2)
		buttonResourceA->cancelObserve();
	else
		button_a_read();
}

static void print_menu()
{
	std::cout << "Demo client menu" << std::endl;
	std::cout << "0  : Print this menu" << std::endl;
	std::cout << "1  : Read sensors" << std::endl;
	std::cout << "2  : Control LEDs" << std::endl;
	std::cout << "3  : Write string to LCD" << std::endl;
	std::cout << "4  : Write buzzer" << std::endl;
	std::cout << "5  : Button control" << std::endl;
	std::cout << "6  : Read sensors (Arduino)" << std::endl;
	std::cout << "7  : Write LED (Arduino)" << std::endl;
	std::cout << "8  : Write string to LCD (Arduino)" << std::endl;
	std::cout << "9  : Write buzzer (Arduino)" << std::endl;
	std::cout << "10 : Button control (Arduino)" << std::endl;
}

static void print_menu_led_p()
{
	std::cout << "1 : Turn on red LED" << std::endl;
	std::cout << "2 : Turn on green LED" << std::endl;
	std::cout << "3 : Turn on blue LED" << std::endl;
	std::cout << "4 : Turn off red LED" << std::endl;
	std::cout << "5 : Turn off green LED" << std::endl;
	std::cout << "6 : Turn off blue LED" << std::endl;
	std::cout << "7 : Read LED status" << std::endl;
}

static void print_menu_led_a()
{
	std::cout << "Enter LED value" << std::endl;
}

static void print_menu_lcd()
{
	std::cout << "Enter string" << std::endl;
}

static void print_menu_buzzer_p()
{
	std::cout << "Enter how long to beep" << std::endl;
}

static void print_menu_buzzer_a()
{
	std::cout << "Enter tone to beep" << std::endl;
}

static void print_menu_button_p()
{
	std::cout << "1 : Register button status" << std::endl;
	std::cout << "2 : De-register button status" << std::endl;
	std::cout << "3 : Read button status" << std::endl;
}

static void print_menu_button_a()
{
	std::cout << "1 : Register button status" << std::endl;
	std::cout << "2 : De-register button status" << std::endl;
	std::cout << "3 : Read button status" << std::endl;
}

void *find_all_resource(void *)
{
	std::ostringstream requestURI;

	// Raspberry Pi 2 server
	std::string sensor_p_rt = "?rt=grovepi.sensor";
	std::string led_p_rt = "?rt=grovepi.led";
	std::string lcd_p_rt = "?rt=grovepi.lcd";
	std::string buzzer_p_rt = "?rt=grovepi.buzzer";
	std::string button_p_rt = "?rt=grovepi.button";

	// Arduino server
	std::string sensor_a_rt = "?rt=grove.sensor";
	std::string led_a_rt = "?rt=grove.led";
	std::string lcd_a_rt = "?rt=grove.lcd";
	std::string buzzer_a_rt = "?rt=grove.buzzer";
	std::string button_a_rt = "?rt=grove.button";

	while(!sensorResourceP || !ledResourceP || !lcdResourceP || !buzzerResourceP || !buttonResourceP
		|| !sensorResourceA || !ledResourceA || !lcdResourceA || !buzzerResourceA || !buttonResourceA) 
	{

		// Find all resources
		if(!sensorResourceP) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << sensor_p_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name2 << " Finding Sensor Resource... " << std::endl;
		}

		if(!ledResourceP) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << led_p_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name2 << " Finding LED Resource... " << std::endl;
		}

		if(!lcdResourceP) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << lcd_p_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name2 << " Finding LCD Resource... " << std::endl;
		}

		if(!buzzerResourceP) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << buzzer_p_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name2 << " Finding Buzzer Resource... " << std::endl;
		}

		if(!buttonResourceP) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << button_p_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name2 << " Finding Button Resource... " << std::endl;
		}

#if 0
		if(!sensorResourceA) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << sensor_a_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name1 << " Finding Sensor Resource... " << std::endl;
		}
#endif

		if(!ledResourceA) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << led_a_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name1 << " Finding LED Resource... " << std::endl;
		}

		if(!lcdResourceA) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << lcd_a_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name1 << " Finding LCD Resource... " <<  std::endl;
		}

		if(!buzzerResourceA) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << buzzer_a_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name1 << " Finding Buzzer Resource... " << std::endl;
		}

		if(!buttonResourceA) {
			requestURI.str("");
			requestURI << OC_RSRVD_WELL_KNOWN_URI << button_a_rt;
			OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
			std::cout << mydemo.resource_name1 << " Finding Button Resource... " << std::endl;
		}

		sleep(8);
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	OCPersistentStorage ps {client_open, fread, fwrite, fclose, unlink };

	if(argc >= 2) {
		mydemo.influxdb_ip = argv[1];
		std::cout << "Infruxdb_ip: " << mydemo.influxdb_ip << std::endl;
		if(argc == 3 && argv[2][0] == '1')
			mydemo.debug_mode = 1;
	} else {
		printUsage();
		return 0;
	}

	std::cout << "Configuring ... ";
	// Create PlatformConfig object
	PlatformConfig cfg {
		OC::ServiceType::InProc,
		OC::ModeType::Both,
		"0.0.0.0",
		0,
		OC::QualityOfService::LowQos,
		&ps
	};

	OCPlatform::Configure(cfg);
	std::cout << "done" << std::endl;


	std::cout << "Start server" << std::endl;
	mydemo.createResource();

	std::cout << "Start client" << std::endl;
	pthread_t tid;
	pthread_create(&tid, NULL, find_all_resource, NULL);

	double curr_light = mydemo.sensor_p_light;
	//double curr_temp = mydemo.sensor_temp;
	double curr_temp = mydemo.sensor_p_temp = 100;


	while(true) {
		if(mydemo.debug_mode) {
			int cmd, cmd1;
			std::string str;
			double buzz_time;
			int tone;
			print_menu();
			std::cin >> cmd;
			switch(cmd) {
				case 0:
					break;
				case 1:
					if(!sensorResourceP) {
						std::cout << "Sensor resource not found" << std::endl;
					} else {
						sensor_p_read();
					}
					break;
				case 2:
					if(!ledResourceP) {
						std::cout << "LED resource not found" << std::endl;
					} else {
						print_menu_led_p();
						std::cin >> cmd1;
						if(cmd1 >=1 && cmd1 <=6)
							led_p_write(cmd1);
						else if(cmd1 == 7)
							led_p_read();
						else
							std::cout << "Unknown option: " << cmd1 << std::endl;
					}
					break;
				case 3:
					if(!lcdResourceP) {
						std::cout << "LCD resource not found" << std::endl;
					} else {
						print_menu_lcd();
						std::cin >> str;
						lcd_p_write(str);
					}
					break;
				case 4:
					if(!buzzerResourceP) {
						std::cout << "Buzzer resource not found" << std::endl;
					} else {
						print_menu_buzzer_p();
						std::cin >> buzz_time;
						buzzer_p_write(buzz_time);
					}
					break;
				case 5:
					if(!buttonResourceP) {
						std::cout << "Button resource not found" << std::endl;
					} else {
						print_menu_button_p();
						std::cin >> cmd1;
						if(cmd1 < 1 && cmd1 > 3)
							std::cout << "Unknown option: " << cmd1 << std::endl;
						else if(cmd1 == 1 || cmd1 == 2)
							button_p_register(cmd1);
						else
							button_p_read();
					}
					break;
				case 6:
					if(!sensorResourceA) {
						std::cout << "Sensor resource not found (Arduino)" << std::endl;
					} else {
						sensor_a_read();
					}
					break;
				case 7:
					if(!ledResourceA) {
						std::cout << "LED resource not found (Arduino)" << std::endl;
					} else {
						print_menu_led_a();
						std::cin >> cmd1;
						led_a_write(cmd1);
					}
					break;
				case 8:
					if(!lcdResourceA) {
						std::cout << "LCD resource not found (Arduino)" << std::endl;
					} else {
						print_menu_lcd();
						std::cin >> str;
						lcd_a_write(str);
					}
					break;
				case 9:
					if(!buzzerResourceA) {
						std::cout << "Buzzer resource not found (Arduino)" << std::endl;
					} else {
						print_menu_buzzer_a();
						std::cin >> tone;
						buzzer_a_write(tone);
					}
					break;
				case 10:
					if(!buttonResourceA) {
						std::cout << "Button resource not found (Arduino)" << std::endl;
					} else {
						print_menu_button_a();
						std::cin >> cmd1;
						if(cmd1 < 1 && cmd1 > 3)
							std::cout << "Unknown option: " << cmd1 << std::endl;
						else if(cmd1 == 1 || cmd1 == 2)
							button_a_register(cmd1);
						else
							button_a_read();
					}
					break;
				default:
					std::cout << "Unknown option: " << cmd << std::endl;
					break;
			}
		} else {
			if(sensorResourceP && ledResourceP) {
				sensor_p_read();
				sleep(2);
			
				if(mydemo.sensor_p_light < 580 && curr_light >= 580) {
					std::cout << "Light status change" << std::endl;
					led_p_write(3);
					curr_light = mydemo.sensor_p_light;
				} else if(mydemo.sensor_p_light >= 580 && curr_light < 580) {
					std::cout << "Light status change" << std::endl;
					led_p_write(6);
					curr_light = mydemo.sensor_p_light;
				}
	
				if(mydemo.sensor_p_temp < 25 && curr_temp >= 25) {
					mydemo.led_p_red = 0;
					mydemo.led_p_green = 1;
					putLedRepresentationP(ledResourceP);
					curr_temp = mydemo.sensor_p_temp;
				} else if(mydemo.sensor_p_temp >= 25 && curr_temp < 25) {
					mydemo.led_p_red = 1;
					mydemo.led_p_green = 0;
					putLedRepresentationP(ledResourceP);
					curr_temp = mydemo.sensor_p_temp;
				}
			}

		}
	}

	return 0;
}
