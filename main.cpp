#include <QDebug>
#include <QFile>
#include <QUdpSocket>
#include <QTcpServer>
#include <QNetworkDatagram>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <signal.h>
#include <unistd.h>

#include "ipfix.h"

static bool ctrl_c = false;

void ignoreUnixSignals(std::initializer_list<int> ignoreSignals) {
	// all these signals will be ignored.
	for (int sig : ignoreSignals)
		signal(sig, SIG_IGN);
}

void catchUnixSignals(std::initializer_list<int> quitSignals) {
	auto handler = [](int sig) -> void {
		// blocking and not aysnc-signal-safe func are valid
		qInfo() << "Signal:" << sig;
		ctrl_c = true;
	};

	sigset_t blocking_mask;
	sigemptyset(&blocking_mask);
	for (auto sig : quitSignals)
		sigaddset(&blocking_mask, sig);

	struct sigaction sa;
	sa.sa_handler = handler;
	sa.sa_mask    = blocking_mask;
	sa.sa_flags   = 0;

	for (auto sig : quitSignals)
		sigaction(sig, &sa, nullptr);
}

int main(int argc, char *argv[]){
	Q_UNUSED(argc)
	Q_UNUSED(argv)

	catchUnixSignals({SIGQUIT, SIGINT, SIGTERM, SIGHUP});

	QString configFile = ":/ipfix.json";

	if(argc > 1){
		QString p = QString::fromLocal8Bit(argv[1]);
		if(p == "-h"){
			qInfo() << "Usage: " << argv[0] << "configFile|showDefault";
			return(0);
		} else if(p == "showDefault"){
			QFile config(configFile);
			if(config.open(QIODevice::ReadOnly)){
				qInfo().noquote() << QString::fromUtf8(config.readAll());
			}
			return(0);
		} else {
			configFile = p;
		}
	}

	QFile config(configFile);
	if(!config.open(QIODevice::ReadOnly)){
		qInfo() << "Unable to open config file" << configFile;
		return(-1);
	}
	if(config.size() > (1024*1024*8)){
		qInfo() << "Config file too long";
		return(-1);
	}
	QJsonParseError err;
	QJsonDocument jDoc = QJsonDocument::fromJson(config.readAll(),&err);
	if(err.error != err.NoError){
		qInfo() << "Parse config problem:" << err.errorString();
		return(-1);
	}
	if(!jDoc.isObject()){
		qInfo() << "Config is not object";
		return(-1);
	}
	QJsonObject jObj = jDoc.object();
	if(!jObj.contains("port") || !jObj.value("port").isDouble()){
		qInfo() << "Missing or invalid port value";
		return(-1);
	}

	if(!jObj.contains("fields") || !jObj.value("fields").isArray()){
		qInfo() << "Missing or invalid fields value";
		return(-1);
	}

	if(!jObj.contains("queueLimit") || !jObj.value("queueLimit").isDouble()){
		qInfo() << "Missing or invalid queueLimit value";
		return(-1);
	}

	if(!jObj.contains("mode") || !jObj.value("mode").isString()){
		qInfo() << "Missing or invalid mode value";
		return(-1);
	}


	int queueLimit = jObj.value("queueLimit").toInt(8192);
	bool debug = jObj.value("debug").toBool(false);
	bool mikrotikFixTimestamp = jObj.value("mikrotikFixTimestamp").toBool(false);
	QString mode = jObj.value("mode").toString("udp");
	IPFIX ipfix(jObj.value("fields").toArray(),queueLimit,mikrotikFixTimestamp,debug);


	if(mode == "udp"){
		QUdpSocket udp;
		if(!udp.bind(QHostAddress::Any,jObj.value("port").toInt())){
			qInfo() << "Unable to bind to port:" << jObj.value("port");
			return(-1);
		}
		quint64 counter = 0;
		while(true){
			if(udp.waitForReadyRead(1000)){
				QNetworkDatagram datagram = udp.receiveDatagram();
				if(datagram.isValid()){
					if(debug) qInfo() << "Datagram:" << datagram.senderAddress() << datagram.senderPort();
					ipfix.enqueue(datagram.data(),datagram.senderAddress());
					if(debug) qInfo() << counter++;
				}
			}
			if(ctrl_c){
				break;
			}
		}
	} else if(mode == "tcp"){
		qInfo() << "Tcp Mode is not implemented yet";
	} else {
		qInfo() << "Unsupported mode" << mode;
	}
	if(ipfix.isRunning()){
		ipfix.requestInterruption();
		qInfo() << "Waiting for ipfix to finish";
		ipfix.wait(5000);
	}
	return(0);
}
