/**
 * @brief Manage mDNS server.
 */


/*
class MDNS {
	public:
		MDNS();
		~MDNS();
		void serviceAdd(const std::string& service, const std::string& proto, uint16_t port);
		void serviceInstanceSet(const std::string& service, const std::string& proto, const std::string& instance);
		void servicePortSet(const std::string& service, const std::string& proto, uint16_t port);
		void serviceRemove(const std::string& service, const std::string& proto);
		void setHostname(const std::string& hostname);
		void setInstance(const std::string& instance);
		// If we the above functions with a basic char*, a copy would be created into an std::string,
		// making the whole thing require twice as much processing power and speed
		void serviceAdd(const char* service, const char* proto, uint16_t port);
		void serviceInstanceSet(const char* service, const char* proto, const char* instance);
		void servicePortSet(const char* service, const char* proto, uint16_t port);
		void serviceRemove(const char* service, const char* proto);
		void setHostname(const char* hostname);
		void setInstance(const char* instance);
	private:
		mdns_server_t* m_mdns_server = nullptr;
};
*/
