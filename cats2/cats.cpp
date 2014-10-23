#include "cats.hpp"

cats_t::cats_t(void) : QMainWindow(nullptr) //, m_signals(nullptr) //m_features(nullptr)
{
	QString			path("C:\\Users\\justin\\Documents\\Visual Studio 2013\\Projects\\cats2\\Data\\Processed\\");
	QStringList		parts({ "Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine" });

	m_central		= new QWidget(this);
	m_layout		= new QVBoxLayout(m_central);
	m_tabs			= new QTabWidget(m_central);
	m_text			= new QTextEdit(m_tabs);
	m_menu			= new QMenuBar(this);
	m_status		= new QStatusBar(this);
	m_openDir		= new QAction(this);
	m_openFile		= new QAction(this);
	m_exit			= new QAction(this);
	m_file			= new QMenu(m_menu);
	//m_spectrogram	= new spectrogram_t(this);
	m_watcher		= new QFutureWatcher< void >(this);
	m_db			= new db_t(this);
	

	setMinimumSize(800, 600);
	m_menu->setGeometry(QRect(0, 0, 800, 21));
	m_menu->addAction(m_file->menuAction());
	m_file->addAction(m_openFile);
	m_file->addAction(m_openDir);
	m_file->addSeparator();
	m_file->addAction(m_exit);

	m_file->setTitle("&File");
	m_openFile->setText("&Open File");
	m_openDir->setText("Open &Directory");
	m_exit->setText("E&xit");

	//m_file->setEnabled(false);
	//m_openFile->setEnabled(false);
	//m_openDir->setEnabled(false);

	setWindowTitle("cats");
	resize(800, 600);
	setCentralWidget(m_central);
	m_central->setLayout(m_layout);
	setMenuBar(m_menu);
	setStatusBar(m_status);

	m_layout->addWidget(m_tabs);

	m_tabs->addTab(m_text, "Console Output");
	m_text->setAcceptRichText(false);
	m_text->setReadOnly(true);
	
	append("Initialized GUI.");

	connect(m_openDir, &QAction::triggered, this, &cats_t::openDir);
	connect(m_openFile, &QAction::triggered, this, &cats_t::openFile);
	connect(m_exit, &QAction::triggered, this, &cats_t::exitApplication);
	connect(m_db, &db_t::hasMessage, this, &cats_t::append);
	connect(m_watcher, &QFutureWatcher< void >::finished, this, &cats_t::dbFinished);

	append("Initializing Sample database. This is slow\nAll file related functionality is disabled until its completed.");
	append("Generally speaking, this should be about 20-30 seconds or so.");

	m_start = std::chrono::system_clock::now();

	//QFuture< void > future = QtConcurrent::run(m_db, &db_t::initialize, path, parts);
	//m_watcher->setFuture(future);
	return;
}


cats_t::~cats_t(void)
{
	return;
}

void 
cats_t::append(QString s)
{
	QMutexLocker lck(&m_mutex);
	QString t = m_text->toPlainText();

	if ( !s.endsWith("\n") )
		s += "\n";

	t += s;

	m_text->clear();
	m_text->setPlainText(t);
	return;
}


void 
cats_t::setText(QString s)
{
	QMutexLocker lck(&m_mutex);

	if ( !s.endsWith("\n") )
		s += "\n";

	m_text->clear();
	m_text->insertPlainText(s);
	return;
}

signal_t< double > 
cats_t::decodeFile(QString path)
{
	QFile*						ifile = nullptr;
	QByteArray					ivec;
	std::vector< uint8_t >		fvec;
	signal_vector_t< double >	svec;
	signal_t< double >			ret;

	if ( !path.endsWith(".mp3") ) {
		append("Invalid file type encountered, this application only supports processing MP3 files.");
		append("If this is a valid MP3, then please ensure that the file extension is '.mp3'");
		throw std::invalid_argument("The path provided was an invalid file type");
	}

	ifile = new QFile(path);

	if ( !ifile->open(QIODevice::ReadOnly | QIODevice::Unbuffered) ) {
		append(QString("Failed to open file: " + path));
		throw std::invalid_argument("The path provided was an invalid file type");
	}

	ivec = ifile->readAll();

	if ( !ivec.size() ) {
		append(QString("Failed to read file: " + path));
		throw std::invalid_argument("The path provided was an invalid file type");
	}

	ifile->close();
	delete ifile;

	fvec.resize(ivec.size());
	std::copy(ivec.begin(), ivec.end(), fvec.begin());
	mp3_t::decode(fvec, svec);

	return svec.join();
}

void 
cats_t::encodeFile(signal_t< double >& sig, QString path)
{

	std::vector< uint8_t >	fvec;
	QFile					file(path);
	
	fvec.clear();
	mp3_t::encode(sig, fvec);

	if ( false == file.open(QIODevice::WriteOnly | QIODevice::Unbuffered | QIODevice::Truncate) ) {
		append(QString("Failed to open file: " + path));
		return;
	}

	for ( std::size_t sidx = 0; sidx < fvec.size(); sidx++ )
		file.putChar(fvec[ sidx ]);

	file.flush();
	file.close();
	return;
}

void
cats_t::openFile(void)
{
	QStringList					dirs	= QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
	QString						fn		= QFileDialog::getOpenFileName(this, "Open File", dirs.first(), tr("Supported Audio (*.mp3 *.wav);; All Files (*.*)"));
	QString						cn		= "C:/Users/justin/Documents/Visual Studio 2013/Projects/cats2/Data/Processed/Eight/eight-001f.mp3";
	QFile*						ifile = nullptr;
	QByteArray					ivec;
	std::vector< uint8_t >		fvec;
	signal_vector_t< double >	sigvec;
	challenge_t					signal;

	if ( "" == fn )
		return;

	if ( !fn.endsWith(".mp3") ) {
		append("Invalid file type encountered, this application only supports processing MP3 files.");
		append("If this is a valid MP3, then please ensure that the file extension is '.mp3'");
		return;
	}

	append("Opening file " + fn + "...");

	/*ifile = new QFile(fn);

	if ( !ifile->open(QIODevice::ReadOnly | QIODevice::Unbuffered) ) {
		append(QString("Failed to open file: " + fn));
		return;
	}

	ivec = ifile->readAll();

	if ( !ivec.size() ) {
		append(QString("Failed to read file: " + fn));
		return;
	}

	fn = ifile->fileName();
	ifile->close();
	delete ifile;

	//append("Decoding MP3...");
	fvec.resize(ivec.size());
	std::copy(ivec.begin(), ivec.end(), fvec.begin());*/

	//m_start = std::chrono::system_clock::now();
	//mp3_t::decode(fvec, sigvec);
	signal_vector_t< double > sv = get_enveloped_signals(decodeFile(fn)); // sigvec.join());
	

	// -- ...
	/*signal_t< double > silence;
	for ( std::size_t j = 0; j < 30 * 11025; j++ )
		silence.push_back(0.0);

	silence.sample_rate(11025);*/

	//filter_noise_t< double > ftn(sv.join().sample_rate());
	//signal_t< double > filtered = ftn.apply(sv.join());
	signal_t< double > noise	= decodeFile("c:\\users\\justin\\desktop\\noise\\noise-000d.mp3");
	signal_t< double > noise2	= decodeFile("c:\\users\\justin\\desktop\\noise\\noise-000c.mp3");
	signal_t< double > sig		= sv.join(); //decodeFile(fn); //sv.join();
	signal_t< double > sigc		= sv.join();

	//for ( std::size_t svi = 0; svi < sv.size(); svi++ ) {
	//	QString path = "c:\\users\\justin\\desktop\\Output\\noisegate-" + QString::number(svi) + ".mp3";
	//	signal_t< double > tsig = noise_gate(sv[ svi ], noise);
	//	encodeFile(noise_gate(tsig, noise2), path);
			//encodeFile(noise_gate(sv[ svi ], noise), path);
			//encodeFile(noise_gate(sv[]))
		encodeFile(noise_gate(sig, noise), "c:\\users\\justin\\desktop\\Output\\noisegate.mp3");
	//}
	//sig = spectral_subtraction(sig, noise, 1000, 0.8);
	/*signal_t< double > nnoise	= sv.join();

	nnoise.sample_rate(sv.join().sample_rate());

	for ( std::size_t nidx = 0; nidx < nnoise.size(); nidx++ )
		nnoise[ nidx ] = ( nnoise[ nidx ] + noise[ nidx % noise.size() - 1 ] );

	encodeFile(nnoise, "c:\\users\\justin\\desktop\\Output\\test4321.mp3");*/

	/*fvec.clear();
	mp3_t::encode(nnoise, fvec);
	QFile ttf("c:\\users\\justin\\desktop\\Output\\test4321.mp3");

	if ( false == ttf.open(QIODevice::WriteOnly | QIODevice::Unbuffered | QIODevice::Truncate) ) {
		append(QString("Failed to open file: " + fn));
		return;
	}

	for ( std::size_t sidx = 0; sidx < fvec.size(); sidx++ )
		ttf.putChar(fvec[ sidx ]);

	ttf.flush();
	ttf.close();*/

	//silence = spectral_subtraction(nnoise, noise, 1000, 0.8);

	/*fvec.clear();
	//silence = get_enveloped_signals(silence).join();
	mp3_t::encode(silence, fvec);
	QFile tf("c:\\users\\justin\\desktop\\Output\\test1234.mp3");

	if ( false == tf.open(QIODevice::WriteOnly | QIODevice::Unbuffered | QIODevice::Truncate) ) {
		append(QString("Failed to open file: " + fn));
		return;
	}

	for ( std::size_t sidx = 0; sidx < fvec.size(); sidx++ )
		tf.putChar(fvec[ sidx ]);

	tf.flush();
	tf.close();*/
	encodeFile(spectral_subtraction(sigc, noise, 1000, 0.8), "c:\\users\\justin\\desktop\\Output\\test1234.mp3");
	return;

	signal.name		= fsearch_t::basename(fn.toStdString());
	signal.sigvec	= sv;

	for ( std::size_t idx = 0; idx < sv.size(); idx++ ) {
		sv[ idx ] = ::level(sv[ idx ]);

		signal.features.push_back(mfcc_t< double >::calculate(sv[ idx ], 20, 12));
	}


	QString guess("");

	for ( std::size_t i = 0; i < signal.features.size(); i++ ) {
		std::vector< signal_mse_t > rets;

		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("Zero")));
		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("One")));
		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("Two")));
		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("Three")));
		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("Four")));
		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("Five")));
		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("Six")));
		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("Seven")));
		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("Eight")));
		rets.push_back(feature_compare_t::mse(signal.features[ i ], m_db->features("Nine")));

		std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - m_start;
		append("Computed challenge file; It took: " + QString::number(elapsed_seconds.count()) + " seconds to complete");
		std::vector< double > vec;

		for ( std::size_t idx = 0; idx < rets.size(); idx++ ) {
			vec.push_back(0.0);

			vec[ idx ] = rets[ idx ].features.sum();
			vec[ idx ] = vec[ idx ] / rets[ idx ].features.size();
		}

		append("Summed Scores for chunk " + QString::number(i) + ": ");
		double min = DBL_MAX;
		std::size_t mindx = 500;

		for ( std::size_t idx = 0; idx < vec.size(); idx++ ) {
			if ( vec[ idx ] < min ) {
				min = vec[idx];
				mindx = idx;
			}

			append(QString::number(idx) + ": " + QString::number(vec[ idx ]) + " Diff Avg: " + 
				   QString::number(::average(rets[idx].differential)) + " Acc Avg: " + 
				   QString::number(::average(rets[idx].acceleration)));

		}

		guess += QString::number(mindx) + " ";
	}

	append("The guess at the captchas value is: " + guess);
/*		hits[ 0 ] = 0;
		hits[ 1 ] = 0;
		hits[ 2 ] = 0;
		hits[ 3 ] = 0;
		hits[ 4 ] = 0;
		hits[ 5 ] = 0;
		hits[ 6 ] = 0;
		hits[ 7 ] = 0;
		hits[ 8 ] = 0;
		hits[ 9 ] = 0;

		for ( auto itr = m_features->begin(); itr != m_features->end(); itr++ ) {
			std::size_t key = 0;
			double mi = DBL_MAX;
			double ma = DBL_MIN;
			double av = 0.0;
			std::size_t avc = 0;

			//append("Comparing segment " + QString::number(idx) + " to samples in the " + itr.key() + " samples bin...");

			if ( !itr.key().compare("Zero", Qt::CaseInsensitive) )
				key = 0;
			else if ( !itr.key().compare("One", Qt::CaseInsensitive) )
				key = 1;
			else if ( !itr.key().compare("Two", Qt::CaseInsensitive) )
				key = 2;
			else if ( !itr.key().compare("Three", Qt::CaseInsensitive) )
				key = 3;
			else if ( !itr.key().compare("Four", Qt::CaseInsensitive) )
				key = 4;
			else if ( !itr.key().compare("Five", Qt::CaseInsensitive) )
				key = 5;
			else if ( !itr.key().compare("Six", Qt::CaseInsensitive) )
				key = 6;
			else if ( !itr.key().compare("Seven", Qt::CaseInsensitive) )
				key = 7;
			else if ( !itr.key().compare("Eight", Qt::CaseInsensitive) )
				key = 8;
			else if ( !itr.key().compare("Nine", Qt::CaseInsensitive) )
				key = 9;

			
			for ( std::size_t bin = 0; bin < itr->size(); bin++ ) {
				QString fn(itr.value().at(bin).first);
				QVector< features_mse_t< double > > featvec;

				for ( std::size_t mseidx = 0; mseidx < feat.size(); mseidx++ ) {
					if ( mseidx >= itr.value().at(bin).second.size() )
						break;

					featvec.push_back(mfcc_t< double >::mse(feat[ mseidx ], itr.value().at(bin).second[ mseidx ]));	
				}
				
				double fs = 0.0;
				double en = 0.0;
				double ds = 0.0;
				double den = 0.0;
				double dds = 0.0;
				double dden = 0.0;

				for ( auto& f : featvec ) {
					fs += f.feature_sum;
					en += f.energy;
					ds += f.differential_sum;
					den += f.differential_energy;
					dds += f.acceleration_sum;
					dden += f.acceleration_energy;
				}

				fs /= featvec.size();
				en /= featvec.size();
				ds /= featvec.size();
				den /= featvec.size();
				dds /= featvec.size();
				dden /= featvec.size();

				append(QString::number(idx) + "," + itr.key() + "," + QString::number(fs) + "," + QString::number(en) + "," + QString::number(ds)
					   + "," + QString::number(den) + "," + QString::number(dds) + "," + QString::number(dden)
					   );

				//append("Segment" + QString::number(idx) + "<->" + fn + " score: " + QString::number(fs) + " Energy:" + QString::number(en)
				//	   + " Delta: " + QString::number(ds) + " Delta Energy: " + QString::number(den) + " Double Delta Score: " +
				//	   QString::number(dds) + " Double Delta Energy: " + QString::number(dden));
					   

				double	rv = mfcc_t< double >::mse(feat, itr.value().at(bin).second.toStdVector());
				double drv = mfcc_t< double >::dmse(feat, itr.value().at(bin).second.toStdVector());
				double ddrv = mfcc_t< double >::ddmse(feat, itr.value().at(bin).second.toStdVector());
				double erv = mfcc_t< double >::emse(feat, itr.value().at(bin).second.toStdVector());
				double edrv = mfcc_t< double >::edmse(feat, itr.value().at(bin).second.toStdVector());
				double eddrv = mfcc_t< double >::eddmse(feat, itr.value().at(bin).second.toStdVector());

				if ( rv > ma )
					ma = rv;
				if ( rv < mi )
					mi = rv;

				if ( 0.19 > rv )
					hits[ key ]++;
				if ( 0.18 > rv )
					hits[ key ]++;
				if ( 0.15 > rv )
					hits[ key ]++;
				if ( 0.13 > rv )
					hits[ key ] += 4;

				av += rv;
				avc++;
				//append("Segment" + QString::number(idx) + "<->" + fn + " score: " + QString::number(rv) + " edrv: " + QString::number(erv));

			}

			av /= avc;
			//append("\n");
			//append("Bin " + itr.key() + " min score: " + QString::number(mi) + " max score: " + QString::number(ma) + " average score: " + QString::number(av) +"\n\n");
			
		}
		std::size_t maxkey = 0;
		std::size_t hkey = 0;

		for ( std::size_t kidx = 0; kidx < hits.size(); kidx++ )
			if ( hits[ kidx ] > maxkey ) {
				maxkey = hits[ kidx ];
				hkey = kidx;
			}

		//append("Writing out segment to MP3...\n");

		QFile mf("c:\\users\\justin\\desktop\\Output\\test-" + QString::number(hkey) + "-" + QString::number(idx) + ".mp3");

		if ( false == mf.open(QIODevice::WriteOnly | QIODevice::Unbuffered | QIODevice::Truncate) ) {
			append(QString("Failed to open file: " + fn));
			return;
		}

		for ( std::size_t sidx = 0; sidx < fvec.size(); sidx++ )
			mf.putChar(fvec[ sidx ]);

		mf.flush();
		mf.close();

		guess += QString::number(hkey);
		//append("The number is " + QString::number(hkey));

	}

	append("My guess at the captcha is: " + guess);*/

	return;
}

void
cats_t::openDir(void)
{
	/*QStringList											dirs = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
	QString												dir = QFileDialog::getExistingDirectory(this, "Select Directory", dirs.first(), QFileDialog::Option::ShowDirsOnly);
	std::vector< std::string >							loc;
	QFile*												ifile = nullptr;
	QString												testfile("C:\\Users\\justin\\Documents\\Visual Studio 2013\\Projects\\cats2\\Data\\Processed\\Three\\three-0008.mp3");
	QByteArray											ivec;
	QMap< QString, QVector< features_t< double > > >	fmap;

	if ( "" == dir )
		return;

	typedef QPair< QString, QVector< features_t< double > > > file_feature_pair_t;
	typedef QVector< file_feature_pair_t > ff_vec_t;
	typedef QMap< QString, ff_vec_t > feature_map_t;

	QString			pbase("C:\\Users\\justin\\Documents\\Visual Studio 2013\\Projects\\cats2\\Data\\Processed\\");
	QStringList		pdirs({ "Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine" });
	feature_map_t	featmap;

	for ( auto& dir : pdirs ) {

		
		try {
			loc = fsearch_t::find_files(QString(pbase + dir).toStdString());

		} catch ( std::exception& e ) {
			append(QString("Error while searching directory: ") + QString(e.what()));
			return;
		}

		for ( auto& f : loc ) {
			QString												fname(f.c_str());
			std::vector< uint8_t >								fvec;
			signal_vector_t< double >							sigvec;

			if ( !fname.endsWith(".mp3") )
				continue;

			ifile = new QFile(fname);

			if ( !ifile->open(QIODevice::ReadOnly | QIODevice::Unbuffered) ) {
				append(QString("Failed to open file: " + fname));
				return;
			}

			ivec = ifile->readAll();

			if ( !ivec.size() ) {
				append(QString("Failed to read file: '" + fname + "'"));
				return;
			}

			ifile->close();
			delete ifile;
			fvec.resize(ivec.size());
			std::copy(ivec.begin(), ivec.end(), fvec.begin());

			mp3_t::decode(fvec, sigvec);
			signal_t< double > s = filter_silence(sigvec.join(), 0);
			s = ::level(s);
			featmap[ dir ].push_back(qMakePair(fname, QVector< features_t< double > >::fromStdVector(mfcc_t< double >::calculate(s, 10, 13))));
		}

		append("Added " + QString::number(featmap[ dir ].size()) + " samples for the number " + dir);
	}*/

/*	append("Searching directory " + dir + " for MP3s...");

	try {
		loc = fsearch_t::find_files(dir.toStdString());
	} catch ( std::exception& e ) {
		append(QString("Error while searching directory: ") + QString(e.what()));
		return;
	}

	for ( auto& f : loc ) {
		QString												fname(f.c_str());
		std::vector< uint8_t >								fvec;
		signal_vector_t< double >							sigvec;
		
		if ( !fname.endsWith(".mp3") )
			continue;

		ifile = new QFile(fname);

		if ( !ifile->open(QIODevice::ReadOnly | QIODevice::Unbuffered) ) {
			append(QString("Failed to open file: " + fname));
			return;
		}

		ivec = ifile->readAll();

		if ( !ivec.size() ) {
			append(QString("Failed to read file: '" + fname + "'"));
			return;
		}

		ifile->close();
		delete ifile;
		fvec.resize(ivec.size());
		std::copy(ivec.begin(), ivec.end(), fvec.begin());

		mp3_t::decode(fvec, sigvec);
		signal_t< double > s = filter_silence(sigvec.join(), 0);
		s = ::level(s);
		fmap.insert(fname, QVector< features_t< double > >::fromStdVector(mfcc_t< double >::calculate(s, 10, 13)));
	}

	std::vector< uint8_t >		tvec;
	signal_vector_t< double >	svec;

	ifile = new QFile(testfile);

	if ( !ifile->open(QIODevice::ReadOnly | QIODevice::Unbuffered) ) {
		append(QString("Failed to open file: " + testfile));
		return;
	}

	ivec = ifile->readAll();

	if ( !ivec.size() ) {
		append(QString("Failed to read file: '" + testfile + "'"));
		return;
	}

	ifile->close();
	delete ifile;
	tvec.resize(ivec.size());
	std::copy(ivec.begin(), ivec.end(), tvec.begin());

	mp3_t::decode(tvec, svec);
	signal_t< double > s = filter_silence(svec.join(), 0);
	s = ::level(s);
	fmap.insert(fmap.begin(), testfile, QVector< features_t< double > >::fromStdVector(mfcc_t< double >::calculate(s, 10, 13)));

	for ( auto itr = fmap.begin(); itr != fmap.end(); itr++ ) {
		QVector< features_mse_t< double > >		diffs;
		QString									name	= itr.key();
		QVector< features_t< double > >&		sample = fmap.last(); //itr.value();

		for ( auto sitr = fmap.begin(); sitr != fmap.end(); sitr++ ) {
			QVector< features_t< double > >& csample = sitr.value();
			double value = mfcc_t< double >::mse(sample.toStdVector(), csample.toStdVector());
			std::cout << "diff: " << value << std::endl;

			for ( std::size_t idx = 0; idx < sample.size(); idx++ )
				if ( idx >= csample.size() )
					break;
				else {
					diffs.push_back(mfcc_t< double >::mse(sample[ idx ], csample[ idx ]));
				}
		}

		std::cout << diffs[ 0 ].delta_energy;

	}*/


	return;
}

void
cats_t::exitApplication(void)
{
	QCoreApplication::quit();
}

void
cats_t::updateStatus(QString m, signed int to)
{
	QMutexLocker lck(&m_mutex);

	m_status->showMessage(m, to);
	return;
}

void
cats_t::hasError(QString m)
{
	QMutexLocker lck(&m_mutex);

	updateStatus(m);
	QMessageBox::warning(this, m, m, QMessageBox::Ok);
	return;
}

void
cats_t::dbFinished(void) 
{
	std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - m_start;

	append("Done processing sample database, enabling file based functionality...");
	append("It took: " + QString::number(elapsed_seconds.count()) + " seconds to complete");
	
	m_file->setEnabled(true);
	m_openFile->setEnabled(true);
	m_openDir->setEnabled(true);

	return;
}
