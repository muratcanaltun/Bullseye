#include <grpcpp/grpcpp.h>
#include <string>
#include "bullseyeindexservice.grpc.pb.h"
#include <curl/curl.h>
#include <rapidjson/document.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using bullseyeindexservice::IndexReply;
using bullseyeindexservice::IndexRequest;
using bullseyeindexservice::IndexCalc;

size_t write_to_string(void* contents, size_t size, size_t nmemb, std::string* s);

class IndexCalcServiceImplementation final : public IndexCalc::Service
{

	Status sendRequest(ServerContext* context, const IndexRequest* request, IndexReply* reply) override
	{
		CURL* curl;
		CURLcode res;
		const char djia[] = "https://query1.finance.yahoo.com/v7/finance/quote?symbols=CVX,MRK,AMGN,MCD,JNJ,KO,CAT,TRV,V,PG,GS,HON,WMT,JPM,CSCO,IBM,DOW,VZ,MMM,HD,UNH,AXP,BA,NKE,CRM,DIS,AAPL,WBA,INTC,MSFT";
		const char spy500[] = "https://query1.finance.yahoo.com/v7/finance/quote?symbols=ABT,ABBV,ABMD,ACN,ATVI,ADBE,AMD,AAP,AES,AFL,A,APD,AKAM,ALK,ALB,ARE,ALGN,ALLE,ALL,AMZN,AMCR,AEE,AAL,AEP,AXP,AIG,AMT,AWK,AMP,ABC,AME,AMGN,APH,ADI,ANSS,ANTM,AON,AOS,APA,AAPL,AMAT,APTV,ADM,ANET,AJG,AIZ,ATO,ADSK,ADP,AZO,AVB,AVY,AVGO,BKR,BLL,BAC,BBWI,BAX,BDX,BRK.B,BBY,BIO,BIIB,BLK,BK,BA,BKNG,BWA,BXP,BSX,BMY,BR,BRO,BF.B,BEN,CHRW,CDNS,CZR,CPB,COF,CAH,CCL,CARR,CTLT,CAT,CBOE,CBRE,CDW,CE,CNC,CNP,CDAY,CERN,CF,CRL,CHTR,CVX,CMG,CB,CHD,CI,CINF,CTAS,CSCO,C,CFG,CTXS,CLX,CME,CMS,CTSH,CL,CMCSA,CMA,CAG,COP,COO,CPRT,CTVA,COST,CTRA,CCI,CSX,CMI,CVS,CRM,DHI,DHR,DRI,DVA,DE,DAL,DVN,DXCM,DLR,DFS,DISCA,DISCK,DISH,DG,DLTR,D,DPZ,DOV,DOW,DTE,DUK,DRE,DD,DXC,DGX,DIS,ED,EMN,ETN,EBAY,ECL,EIX,EW,EA,EMR,ENPH,ETR,EOG,EFX,EQIX,EQR,ESS,EL,ETSY,EVRG,ES,EXC,EXPE,EXPD,EXR,FANG,FFIV,FB,FAST,FRT,FDX,FIS,FITB,FE,FRC,FISV,FLT,FMC,F,FTNT,FTV,FBHS,FOXA,FOX,FCX,GOOGL,GOOG,GLW,GPS,GRMN,GNRC,GD,GE,GIS,GM,GPC,GILD,GL,GPN,GS,GWW,HAL,HBI,HIG,HAS,HCA,HSIC,HSY,HES,HPE,HLT,HOLX,HD,HON,HRL,HST,HWM,HPQ,HUM,HBAN,HII,IT,IEX,IDXX,INFO,ITW,ILMN,INCY,IR,INTC,ICE,IBM,IP,IPG,IFF,INTU,ISRG,IVZ,IPGP,IQV,IRM,JKHY,J,JBHT,JNJ,JCI,JPM,JNPR,KMX,KO,KSU,K,KEY,KEYS,KMB,KIM,KMI,KLAC,KHC,KR,LNT,LHX,LH,LRCX,LW,LVS,LEG,LDOS,LEN,LLY,LNC,LIN,LYV,LKQ,LMT,L,LOW,LUMN,LYB,LUV,MMM,MO,MTB,MRO,MPC,MKTX,MAR,MMC,MLM,MAS,MA,MTCH,MKC,MCD,MCK,MDT,MRK,MET,MTD,MGM,MCHP,MU,MSFT,MAA,MRNA,MHK,MDLZ,MPWR,MNST,MCO,MS,MOS,MSI,MSCI,NDAQ,NTAP,NFLX,NWL,NEM,NWSA,NWS,NEE,NLSN,NKE,NI,NSC,NTRS,NOC,NLOK,NCLH,NRG,NUE,NVDA,NVR,NXPI,NOW,ORLY,OXY,ODFL,OMC,OKE,ORCL,OGN,OTIS,O,PEAK,PCAR,PKG,PH,PAYX,PAYC,PYPL,PENN,PNR,PBCT,PEP,PKI,PFE,PM,PSX,PNW,PXD,PNC,POOL,PPG,PPL,PFG,PG,PGR,PLD,PRU,PTC,PEG,PSA,PHM,PVH,PWR,QRVO,QCOM,RE,RL,RJF,RTX,REG,REGN,RF,RSG,RMD,RHI,ROK,ROL,ROP,ROST,RCL,SCHW,STZ,SJM,SPGI,SBAC,SLB,STX,SEE,SRE,SHW,SPG,SWKS,SNA,SO,SWK,SBUX,STT,STE,SYK,SIVB,SYF,SNPS,SYY,T,TECH,TAP,TMUS,TROW,TTWO,TPR,TGT,TEL,TDY,TFX,TER,TSLA,TXN,TXT,TMO,TJX,TSCO,TT,TDG,TRV,TRMB,TFC,TWTR,TYL,TSN,UDR,ULTA,USB,UAA,UA,UNP,UAL,UNH,UPS,URI,UHS,VLO,VTR,VRSN,VRSK,VZ,VRTX,VFC,VIAC,VTRS,V,VNO,VMC,WRB,WAB,WMT,WBA,WM,WAT,WEC,WFC,WELL,WST,WDC,WU,WRK,WY,WHR,WMB,WLTW,WYNN,XRAY,XOM,XEL,XLNX,XYL,YUM,ZBRA,ZBH,ZION,ZTS";


		int index_id = request->index_id();
		double index_divisor = 1;

		curl = curl_easy_init();
		std::string stringify;

		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		switch (index_id)
		{
		case 2:
			index_divisor = 8452;
			if (curl) {
				curl_easy_setopt(curl, CURLOPT_URL, spy500);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stringify);

				res = curl_easy_perform(curl);

				curl_easy_cleanup(curl);
			}
			break;
		default:
			index_divisor = 0.15;
			if (curl) {
				curl_easy_setopt(curl, CURLOPT_URL, djia);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stringify);

				res = curl_easy_perform(curl);

				curl_easy_cleanup(curl);
			}
			break;
		}
		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> time_spent = t2 - t1;
		std::cout << "Fetched stock data in " << time_spent.count() << " ms." << std::endl;

		rapidjson::Document doc;
		doc.Parse<0>(stringify.c_str());

		assert(doc.IsObject());

		rapidjson::Value& quoteResponse = doc["quoteResponse"];
		rapidjson::Value& result = quoteResponse["result"];

		assert(result.IsArray());

		double price = 0;

		for (auto const& p : quoteResponse["result"].GetArray()) {
			price += p["regularMarketPrice"].GetDouble();
		}

		price = price / index_divisor;

		reply->set_index_value(price);
		return Status::OK;
	}
};

void RunServer()
{
	std::string server_address("127.0.0.1:50051");
	IndexCalcServiceImplementation service;

	ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Bullseye Index Service listening on: " << server_address << std::endl;

	server->Wait();
}

size_t write_to_string(void* contents, size_t size, size_t nmemb, std::string* s)
{
	size_t newLength = size * nmemb;
	try
	{
		s->append((char*)contents, newLength);
	}
	catch (std::bad_alloc& e)
	{
		//handle memory problem
		return 0;
	}
	return newLength;
}

int main()
{
	RunServer();
}
