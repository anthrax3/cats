#ifndef DB_HPP
#define DB_HPP

#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QVector>
#include <QPair>
#include <QStringList>
#include <QByteArray>

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <exception>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <memory>

#include "signal.hpp"
#include "mfcc.hpp"
#include "fsearch.hpp"
#include "mp3.hpp"
#include "mel_filter.hpp"

struct signal_sample_t
{
	std::string								name;
	signal_t< double >						signal;
	std::vector< features_t< double > >		features;

	signal_sample_t(void) : name("") { return; }
	
	virtual ~signal_sample_t(void)
	{
		features.clear();
		return;
	}
};

typedef std::vector< signal_sample_t* > signal_samples_vector_t;
typedef signal_samples_vector_t* samples_ptr_t;


struct challenge_t
{
	std::string								name;
	std::string								path;
	signal_vector_t< double >				sigvec;
	std::vector< features_vector_t< double > > features;
};

typedef QPair< QString, QVector< features_t< double > > > file_feature_pair_t;
typedef QVector< file_feature_pair_t > ff_vec_t;
typedef QMap< QString, ff_vec_t > feature_map_t;

class db_t : public QObject
{
	Q_OBJECT

	signals:
		void hasMessage(const QString&);
		void finished(void);

	private slots:
		void append(const QString&);
		void append(const char*);

	private:
		QMap< QString, std::vector< signal_sample_t > >		m_features;
		//std::vector< signal_sample_t >						m_samples;
		//feature_map_t										m_features;
		std::mutex											m_mutex;
		bool												m_initialized;

	protected:
	public:
		db_t(QObject* p = nullptr);
		~db_t(void);
		void initialize(QString, QStringList);
		QMap< QString, std::vector< signal_sample_t > >& features(void);
		std::vector< signal_sample_t >& features(QString&);
		std::vector< signal_sample_t >& features(const char*);
		signal_samples_vector_t* run(QString, QStringList);
};

#endif // DB_HPP
