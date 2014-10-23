#include "db.hpp"

db_t::db_t(QObject* p) : QObject(p)
{

	
	return;
}

db_t::~db_t(void)
{
	std::lock_guard< std::mutex > l(m_mutex);

	for ( auto& f : m_features )
		f.clear();

	m_features.clear();
	return;
}

void 
db_t::append(const QString& m) 
{
	std::lock_guard< std::mutex > l(m_mutex);

	emit hasMessage(m);
	
	return;
}

void 
db_t::append(const char* m)
{
	std::lock_guard< std::mutex > l(m_mutex);

	emit hasMessage(QString(m));

	return;
}

void 
db_t::initialize(QString path, QStringList parts)
{
	QVector< std::thread* >			tv;

	for ( auto& dir : parts ) {
		tv.push_back(new std::thread([&]()
		{
			std::vector< uint8_t >			fvec;
			signal_vector_t< double >		sigvec;
			std::vector< std::string >		loc;
			QFile*							ifile = nullptr;
			QByteArray						ivec;

			loc = fsearch_t::find_files(QString(path + dir).toStdString());

			for ( auto& f : loc ) {
				QString									fname(f.c_str());
				std::vector < features_t< double > >	tmp_vec;
				signal_sample_t							signal;

				if ( !fname.endsWith(".mp3") )
					continue;

				ifile = new QFile(fname);

				if ( !ifile->open(QIODevice::ReadOnly | QIODevice::Unbuffered) ) {
					append("Failed to open file: " + fname);
					throw std::runtime_error("Failed to open file");
				}

				ivec = ifile->readAll();

				if ( !ivec.size() ) {
					append("Failed to read file: '" + fname + "'");
					throw std::runtime_error("Failed to read file");
				}

				ifile->close();
				delete ifile;
				fvec.resize(ivec.size());
				std::copy(ivec.begin(), ivec.end(), fvec.begin());

				// -- of course LAME isn't thread safe...
				m_mutex.lock();
				mp3_t::decode(fvec, sigvec);
				m_mutex.unlock();

				signal_t< double > s = filter_silence(sigvec.join(), 0);
				s = ::level(s);

				signal.features = mfcc_t< double >::calculate(s, 20, 12, &m_mutex);

				m_mutex.lock();
				m_features[ dir ].push_back(signal);
				m_mutex.unlock();
			}

		}));
	}

	for ( auto& t : tv ) {
		t->join();
		delete t;
	}

	m_initialized = true;
	//emit finished();
	return;
}

QMap< QString, std::vector< signal_sample_t > >& 
db_t::features(void)
{
	std::lock_guard< std::mutex > l(m_mutex);

	if ( false == m_initialized )
		throw std::runtime_error("Attempt to retrieve the features set prior to their being fully initialized.");

	return m_features;
}


std::vector< signal_sample_t >& 
db_t::features(QString& t)
{
	std::lock_guard< std::mutex > l(m_mutex);

	if ( false == m_initialized )
		throw std::runtime_error("Attempt to retrieve the features set prior to their being fully initialized.");

	for ( auto itr = m_features.begin(); itr != m_features.end(); itr++ )
		if (! itr.key().compare(t, Qt::CaseInsensitive) )
			return itr.value();

	throw std::runtime_error("Requested Feature bin that does not exist.");
}

std::vector< signal_sample_t >&
db_t::features(const char* t)
{
	return features(QString(t));
}

samples_ptr_t
db_t::run(QString path, QStringList parts)
{
//	QVector< std::thread* >			tv;
	signal_samples_vector_t*		svec(new signal_samples_vector_t());
	//fft_t< double >*				fft(fft_factory_t< double >::getFFT());
	//std::size_t						sr = 0;
	//std::size_t						sz = 0;
	//mel_filter_bank_t< double >*	bank(nullptr);	//bank(frames.getFrame(0).sample_rate(), frames.getFrame(0).size());

/*	for ( auto& dir : parts ) {

		tv.push_back(new std::thread([&]()
		{
			std::vector< uint8_t >			fvec;
			signal_vector_t< double >		sigvec;
			std::vector< std::string >		loc;
			QFile*							ifile = nullptr;
			QByteArray						ivec;
			

			loc = fsearch_t::find_files(QString(path + dir).toStdString());
	
			for ( auto& f : loc ) {
				QString									fname(f.c_str());
				std::vector < features_t< double >* >	tmp_vec;
				signal_sample_t*						ptr(nullptr);

				if ( !fname.endsWith(".mp3") )
					continue;

				ifile = new QFile(fname);

				if ( !ifile->open(QIODevice::ReadOnly | QIODevice::Unbuffered) ) {
					append("Failed to open file: " + fname);
					throw std::runtime_error("Failed to open file");
				}

				ivec = ifile->readAll();

				if ( !ivec.size() ) {
					append("Failed to read file: '" + fname + "'");
					throw std::runtime_error("Failed to read file");
				}

				ifile->close();
				delete ifile;
				fvec.resize(ivec.size());
				std::copy(ivec.begin(), ivec.end(), fvec.begin());

				// -- of course LAME isn't thread safe...
				m_mutex.lock();
				mp3_t::decode(fvec, sigvec);
				m_mutex.unlock();

				signal_t< double > s = filter_silence(sigvec.join(), 0);
				s = ::level(s);
				m_mutex.lock();
				tmp_vec = mfcc_t< double >::calculate(s, 10 , 13);
				m_mutex.unlock();

				ptr = new signal_sample_t();

				ptr->name		= fsearch_t::basename(fname.toStdString());
				ptr->path		= fname.toStdString();
				ptr->type		= dir.toStdString();
				ptr->signal		= new signal_t< double >(s);

				 for ( auto& e : tmp_vec )
					ptr->features.push_back(e);

				m_mutex.lock();
				svec->push_back(ptr);
				ptr = nullptr;
				m_mutex.unlock();

				m_mutex.lock();
				m_features[ dir ].push_back(qMakePair(fname,
					QVector< features_t< double > >::fromStdVector(mfcc_t< double >::calculate(s, 10, 13))));
				m_mutex.unlock();
			}

			//append("Added " + QString::number(m_features[ dir ].size()) + " samples for the number " + dir);
		}));
	}

	for ( auto& t : tv ) {
		t->join();
		delete t;
	}*/

	//append("Added " + QString::number(svec->size()) + " samples.");

	return svec;
}