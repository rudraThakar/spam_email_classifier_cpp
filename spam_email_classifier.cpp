#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <gtk/gtk.h>
#include <algorithm>
#include <cctype>

using namespace std;

// Structure to hold word frequency data
struct WordFreq {
    string word;
    double spamFreq;
    double hamFreq;

    WordFreq(string w = "", double s = 0.0, double h = 0.0)
        : word(w), spamFreq(s), hamFreq(h) {}
};

// Node for chaining hash map
struct Node {
    WordFreq data;
    Node* next;
    Node(WordFreq d) : data(d), next(nullptr) {}
};

// Abstract HashMap class
class HashMap {
protected:
    int size;
    int count;

    int hash(string key) {
        int hashVal = 0;
        for (char c : key)
            hashVal = 37 * hashVal + c;
        return abs(hashVal) % size;
    }

public:
    HashMap(int s = 10007) : size(s), count(0) {}
    virtual ~HashMap() {}

    virtual void insert(WordFreq data) = 0;
    virtual WordFreq* search(string key) = 0;
    virtual void clear() = 0;

    double getLoadFactor() { return (double)count / size; }
    int getCount() { return count; }
};

// Chaining HashMap implementation
class ChainingHashMap : public HashMap {
private:
    vector<Node*> table;

public:
    ChainingHashMap(int s = 10007) : HashMap(s) {
        table.resize(size, nullptr);
    }

    ~ChainingHashMap() {
        clear();
    }

    void insert(WordFreq data) override {
        int index = hash(data.word);
        Node* newNode = new Node(data);

        if (!table[index]) {
            table[index] = newNode;
            count++;
            return;
        }

        Node* current = table[index];
        while (current->next) {
            if (current->data.word == data.word) {
                current->data = data;
                delete newNode;
                return;
            }
            current = current->next;
        }

        if (current->data.word == data.word) {
            current->data = data;
            delete newNode;
            return;
        }

        current->next = newNode;
        count++;
    }

    WordFreq* search(string key) override {
        int index = hash(key);
        Node* current = table[index];

        while (current) {
            if (current->data.word == key)
                return &(current->data);
            current = current->next;
        }
        return nullptr;
    }

    void clear() override {
        for (Node* head : table) {
            while (head) {
                Node* temp = head;
                head = head->next;
                delete temp;
            }
        }
        table.clear();
        count = 0;
    }
};

// Open Addressing HashMap implementation
class OpenAddressingHashMap : public HashMap {
private:
    vector<pair<bool, WordFreq>> table;

public:
    OpenAddressingHashMap(int s = 10007) : HashMap(s) {
        table.resize(size, {false, WordFreq()});
    }

    void insert(WordFreq data) override {
        int index = hash(data.word);
        int i = 0;

        while (i < size) {
            int currentIndex = (index + i) % size;

            if (!table[currentIndex].first) {
                table[currentIndex] = {true, data};
                count++;
                return;
            } else if (table[currentIndex].second.word == data.word) {
                table[currentIndex].second = data;
                return;
            }
            i++;
        }
        cout << "Hash table is full!" << endl;
    }

    WordFreq* search(string key) override {
        int index = hash(key);
        int i = 0;

        while (i < size) {
            int currentIndex = (index + i) % size;

            if (!table[currentIndex].first) return nullptr;
            if (table[currentIndex].second.word == key)
                return &(table[currentIndex].second);
            i++;
        }
        return nullptr;
    }

    void clear() override {
        table.clear();
        table.resize(size, {false, WordFreq()});
        count = 0;
    }
};

// EmailClassifier with probability
class EmailClassifier {
private:
    HashMap* wordMap;
    double threshold;

public:
    EmailClassifier(HashMap* map, double thresh = 0.7)
        : wordMap(map), threshold(thresh) {}

    pair<bool, double> classifyWithProbability(const vector<string>& emailWords) {
        double spamScore = 0.0, totalWords = 0.0;

        for (const string& word : emailWords) {
            WordFreq* wf = wordMap->search(word);
            if (wf) {
                double totalFreq = wf->spamFreq + wf->hamFreq;
                if (totalFreq > 0) {
                    spamScore += (wf->spamFreq / totalFreq);
                    totalWords += 1.0;
                }
            }
        }

        double prob = (totalWords > 0) ? (spamScore / totalWords) : 0.0;
        return {prob >= threshold, prob};
    }

    void setThreshold(double thresh) {
        threshold = thresh;
    }
};

// Utility to split CSV lines
vector<string> splitCSVLine(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string token;

    while (getline(ss, token, ',')) {
        if (!token.empty() && token.front() == '"' && token.back() == '"') {
            token = token.substr(1, token.length() - 2);
        }
        tokens.push_back(token);
    }
    return tokens;
}

// Load word frequencies from CSV
void loadWordFrequenciesFromTransposedCSV(const string& filename, HashMap* chainMap, HashMap* openMap, vector<string>& wordsOrder) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    string wordsLine, spamLine, hamLine;
    getline(file, wordsLine);
    getline(file, spamLine);
    getline(file, hamLine);

    vector<string> words = splitCSVLine(wordsLine);
    vector<string> spamCounts = splitCSVLine(spamLine);
    vector<string> hamCounts = splitCSVLine(hamLine);

    if (spamCounts.size() != words.size() || hamCounts.size() != words.size()) {
        cerr << "Error: Inconsistent number of columns in CSV file" << endl;
        return;
    }

    for (size_t i = 0; i < words.size(); ++i) {
        if (words[i].empty() || words[i] == "Word" || words[i] == "word") continue;

        try {
            double spamFreq = stod(spamCounts[i]);
            double hamFreq = stod(hamCounts[i]);
            WordFreq wordFreq(words[i], spamFreq, hamFreq);
            chainMap->insert(wordFreq);
            openMap->insert(wordFreq);
            wordsOrder.push_back(words[i]);
        } catch (...) {
            cerr << "Error processing column " << i + 1 << ": " << words[i] << endl;
        }
    }

    file.close();
}

// Save updated word frequencies to CSV
void saveWordFrequenciesToTransposedCSV(const string& filename, const vector<string>& wordsOrder, HashMap* wordMap) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file for writing: " << filename << endl;
        return;
    }

    for (size_t i = 0; i < wordsOrder.size(); ++i) {
        if (i > 0) file << ",";
        file << "\"" << wordsOrder[i] << "\"";
    }
    file << endl;

    for (size_t i = 0; i < wordsOrder.size(); ++i) {
        if (i > 0) file << ",";
        WordFreq* wf = wordMap->search(wordsOrder[i]);
        file << (wf ? wf->spamFreq : 0);
    }
    file << endl;

    for (size_t i = 0; i < wordsOrder.size(); ++i) {
        if (i > 0) file << ",";
        WordFreq* wf = wordMap->search(wordsOrder[i]);
        file << (wf ? wf->hamFreq : 0);
    }
    file << endl;

    file.close();
}

// Application data structure
struct AppData {
    GtkWidget* window;
    GtkWidget* textView;
    GtkWidget* classifyButton;
    GtkWidget* clearButton;
    GtkWidget* loadButton;
    GtkWidget* viewDatasetButton;
    GtkWidget* resultLabel;
    GtkWidget* markSpamButton;
    GtkWidget* markHamButton;
    vector<string> wordsOrder;
    ChainingHashMap chainMap;
    OpenAddressingHashMap openMap;
    vector<string> currentEmailWords;
    double spamThreshold; // Added to store threshold
};

// Color functions for highlighting
const char* get_spam_color(int level) {
    switch (level) {
        case 1: return "#FFCCCC"; // light red
        case 2: return "#FF9999";
        case 3: return "#FF6666";
        case 4: return "#FF3333";
        case 5: return "#FF0000"; // dark red
        default: return "#FFFFFF";
    }
}

const char* get_ham_color(int level) {
    switch (level) {
        case 1: return "#CCFFCC"; // light green
        case 2: return "#99FF99";
        case 3: return "#66FF66";
        case 4: return "#33FF33";
        case 5: return "#00FF00"; // dark green
        default: return "#FFFFFF";
    }
}

// Highlight words in the text view
void highlightWords(GtkTextBuffer* buffer, const vector<string>& emailWords, HashMap* wordMap) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_remove_all_tags(buffer, &start, &end);

    gchar* text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    string textStr(text);
    g_free(text);

    size_t pos = 0;
    string word;
    size_t wordStartPos = 0;
    bool inWord = false;

    for (size_t i = 0; i <= textStr.size(); ++i) {
        char c = (i < textStr.size()) ? textStr[i] : ' ';
        if (isalnum(c)) {
            if (!inWord) {
                wordStartPos = i;
                inWord = true;
            }
            word += c;
        } else {
            if (inWord) {
                inWord = false;
                WordFreq* wf = wordMap->search(word);
                if (wf) {
                    double totalFreq = wf->spamFreq + wf->hamFreq;
                    if (totalFreq > 0) {
                        double contribution = (wf->spamFreq - wf->hamFreq) / totalFreq;
                        string tagName;
                        if (contribution > 0) {
                            int level = min(5, static_cast<int>(contribution / 0.2) + 1);
                            tagName = "spam-" + to_string(level);
                            cout << "Word: " << word << ", Contribution: " << contribution << ", Spam Level: " << level << endl;
                        } else if (contribution < 0) {
                            int level = min(5, static_cast<int>(-contribution / 0.2) + 1);
                            tagName = "ham-" + to_string(level);
                            cout << "Word: " << word << ", Contribution: " << contribution << ", Ham Level: " << level << endl;
                        }
                        if (!tagName.empty()) {
                            GtkTextIter wordStart, wordEnd;
                            gtk_text_buffer_get_iter_at_offset(buffer, &wordStart, wordStartPos);
                            gtk_text_buffer_get_iter_at_offset(buffer, &wordEnd, i);
                            gtk_text_buffer_apply_tag_by_name(buffer, tagName.c_str(), &wordStart, &wordEnd);
                        }
                    }
                }
                word.clear();
            }
        }
    }
}

// Classify button callback
void on_classify_button_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->textView));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar* emailText = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    app->currentEmailWords.clear();
    stringstream ss(emailText);
    string word;
    while (ss >> word) {
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        word.erase(remove_if(word.begin(), word.end(), [](char c) { return !isalnum(c); }), word.end());
        if (!word.empty()) {
            app->currentEmailWords.push_back(word);
        }
    }

    EmailClassifier classifier(&app->chainMap, app->spamThreshold);
    pair<bool, double> result = classifier.classifyWithProbability(app->currentEmailWords);
    bool isSpam = result.first;
    double probability = result.second;

    string resultText = isSpam ? "<span color='#D32F2F'>Spam" : "<span color='#388E3C'>Not Spam";
    resultText += " (Probability: " + to_string(probability) + ")</span>";
    gtk_label_set_markup(GTK_LABEL(app->resultLabel), resultText.c_str());

    highlightWords(buffer, app->currentEmailWords, &app->chainMap);

    gtk_widget_set_sensitive(app->markSpamButton, TRUE);
    gtk_widget_set_sensitive(app->markHamButton, TRUE);
    g_free(emailText);
}

// Clear screen button callback
void on_clear_button_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->textView));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_delete(buffer, &start, &end);
    gtk_text_buffer_remove_all_tags(buffer, &start, &end);
    gtk_label_set_text(GTK_LABEL(app->resultLabel), "");
    gtk_widget_set_sensitive(app->markSpamButton, FALSE);
    gtk_widget_set_sensitive(app->markHamButton, FALSE);
    app->currentEmailWords.clear();
}

// Load email from file callback
void on_load_button_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Open Email File",
                                                    GTK_WINDOW(app->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        ifstream file(filename);
        if (file.is_open()) {
            stringstream buffer;
            buffer << file.rdbuf();
            string content = buffer.str();
            file.close();

            GtkTextBuffer* textBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->textView));
            gtk_text_buffer_set_text(textBuffer, content.c_str(), -1);
            gtk_label_set_text(GTK_LABEL(app->resultLabel), "");
            app->currentEmailWords.clear();
            gtk_widget_set_sensitive(app->markSpamButton, FALSE);
            gtk_widget_set_sensitive(app->markHamButton, FALSE);
        } else {
            gtk_label_set_text(GTK_LABEL(app->resultLabel), "Error: Could not open file");
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

// Callback for updating threshold
void on_update_threshold_button_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    GtkWidget* entry = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "threshold_entry"));
    const char* threshold_text = gtk_entry_get_text(GTK_ENTRY(entry));
    try {
        double new_threshold = stod(threshold_text);
        if (new_threshold >= 0.0 && new_threshold <= 1.0) {
            app->spamThreshold = new_threshold;
            gtk_label_set_markup(GTK_LABEL(app->resultLabel), "<span color='#388E3C'>Threshold updated successfully</span>");
        } else {
            gtk_label_set_markup(GTK_LABEL(app->resultLabel), "<span color='#D32F2F'>Threshold must be between 0.0 and 1.0</span>");
        }
    } catch (...) {
        gtk_label_set_markup(GTK_LABEL(app->resultLabel), "<span color='#D32F2F'>Invalid threshold value</span>");
    }
}

// Properties button callback
void on_properties_button_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);

    // Calculate dataset properties
    int totalWords = app->chainMap.getCount();
    double totalSpamFreq = 0.0, totalHamFreq = 0.0;
    string maxSpamWord = "None", maxHamWord = "None";
    double maxSpamFreq = 0.0, maxHamFreq = 0.0;

    for (const string& word : app->wordsOrder) {
        WordFreq* wf = app->chainMap.search(word);
        if (wf) {
            totalSpamFreq += wf->spamFreq;
            totalHamFreq += wf->hamFreq;
            if (wf->spamFreq > maxSpamFreq) {
                maxSpamFreq = wf->spamFreq;
                maxSpamWord = wf->word;
            }
            if (wf->hamFreq > maxHamFreq) {
                maxHamFreq = wf->hamFreq;
                maxHamWord = wf->word;
            }
        }
    }

    string dominantCategory = (totalSpamFreq > totalHamFreq) ? "Spam" :
                             (totalHamFreq > totalSpamFreq) ? "Ham" : "Equal";
    double loadFactor = app->chainMap.getLoadFactor();

    // Create properties text
    stringstream ss;
    ss << "<b>Dataset Properties</b>\n\n"
       << "Total Unique Words: " << totalWords << "\n"
       << "Total Spam Frequency: " << totalSpamFreq << "\n"
       << "Total Ham Frequency: " << totalHamFreq << "\n"
       << "Dominant Category: " << dominantCategory << "\n"
       << "Hash Map Load Factor: " << loadFactor << "\n"
       << "Most Frequent Spam Word: " << maxSpamWord << " (" << maxSpamFreq << ")\n"
       << "Most Frequent Ham Word: " << maxHamWord << " (" << maxHamFreq << ")\n"
       << "Current Spam Threshold: " << app->spamThreshold << "\n";

    // Create dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Dataset Properties",
                                                   GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close",
                                                   GTK_RESPONSE_CLOSE,
                                                   NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 400);

    // Create label with markup
    GtkWidget* label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), ss.str().c_str());
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_widget_set_margin_start(label, 10);
    gtk_widget_set_margin_end(label, 10);
    gtk_widget_set_margin_top(label, 10);
    gtk_widget_set_margin_bottom(label, 10);

    // Create threshold entry and button
    GtkWidget* threshold_label = gtk_label_new("New Spam Threshold (0.0-1.0):");
    GtkWidget* threshold_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(threshold_entry), "e.g., 0.7");
    GtkWidget* update_threshold_button = gtk_button_new_with_label("Update Threshold");
    gtk_widget_set_name(update_threshold_button, "update-threshold-button");

    // Store entry in button's data for callback
    g_object_set_data(G_OBJECT(update_threshold_button), "threshold_entry", threshold_entry);

    // Layout for threshold controls
    GtkWidget* threshold_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(threshold_box), threshold_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(threshold_box), threshold_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(threshold_box), update_threshold_button, FALSE, FALSE, 0);

    // Main layout
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), threshold_box, FALSE, FALSE, 0);

    // Add to dialog content area
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), main_box, TRUE, TRUE, 0);

    // Connect signal for updating threshold
    g_signal_connect(update_threshold_button, "clicked", G_CALLBACK(on_update_threshold_button_clicked), app);

    gtk_widget_show_all(dialog);

    // Run dialog
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Structure to hold filter and sort parameters
struct FilterSortData {
    AppData* app;
    GtkListStore* store;
    GtkWidget* alpha_entry;
    GtkWidget* spam_count_entry;
    GtkWidget* ham_count_entry;
    GtkWidget* spam_score_entry;
    GtkWidget* ham_score_entry;
    GtkWidget* spam_count_combo;
    GtkWidget* ham_count_combo;
    GtkWidget* spam_score_combo;
    GtkWidget* ham_score_combo;
    GtkWidget* sort_combo;
};

// Callback to apply filters and sorting
void on_apply_filter_button_clicked(GtkButton* button, gpointer user_data) {
    FilterSortData* fs_data = static_cast<FilterSortData*>(user_data);
    AppData* app = fs_data->app;
    GtkListStore* store = fs_data->store;

    // Get filter values
    const char* alpha_filter = gtk_entry_get_text(GTK_ENTRY(fs_data->alpha_entry));
    string alpha_filter_str = string(alpha_filter);
    transform(alpha_filter_str.begin(), alpha_filter_str.end(), alpha_filter_str.begin(), ::tolower);

    double spam_count_threshold = 0.0, ham_count_threshold = 0.0;
    double spam_score_threshold = 0.0, ham_score_threshold = 0.0;
    try {
        string spam_count_str = gtk_entry_get_text(GTK_ENTRY(fs_data->spam_count_entry));
        string ham_count_str = gtk_entry_get_text(GTK_ENTRY(fs_data->ham_count_entry));
        string spam_score_str = gtk_entry_get_text(GTK_ENTRY(fs_data->spam_score_entry));
        string ham_score_str = gtk_entry_get_text(GTK_ENTRY(fs_data->ham_score_entry));
        if (!spam_count_str.empty()) spam_count_threshold = stod(spam_count_str);
        if (!ham_count_str.empty()) ham_count_threshold = stod(ham_count_str);
        if (!spam_score_str.empty()) spam_score_threshold = stod(spam_score_str);
        if (!ham_score_str.empty()) ham_score_threshold = stod(ham_score_str);
    } catch (...) {
        // Ignore invalid numbers
    }

    bool spam_count_above = gtk_combo_box_get_active(GTK_COMBO_BOX(fs_data->spam_count_combo)) == 0;
    bool ham_count_above = gtk_combo_box_get_active(GTK_COMBO_BOX(fs_data->ham_count_combo)) == 0;
    bool spam_score_above = gtk_combo_box_get_active(GTK_COMBO_BOX(fs_data->spam_score_combo)) == 0;
    bool ham_score_above = gtk_combo_box_get_active(GTK_COMBO_BOX(fs_data->ham_score_combo)) == 0;

    // Get sort criterion
    int sort_criterion = gtk_combo_box_get_active(GTK_COMBO_BOX(fs_data->sort_combo));

    // Collect and filter words
    vector<pair<string, WordFreq>> filtered_words;
    for (const string& word : app->wordsOrder) {
        WordFreq* wf = app->chainMap.search(word);
        if (!wf) continue;

        // Apply alphabetical filter
        string word_lower = word;
        transform(word_lower.begin(), word_lower.end(), word_lower.begin(), ::tolower);
        if (!alpha_filter_str.empty() && word_lower.find(alpha_filter_str) == string::npos) {
            continue;
        }

        // Apply numerical filters
        double spam_score = (wf->spamFreq + wf->hamFreq > 0) ? wf->spamFreq / (wf->spamFreq + wf->hamFreq) : 0.0;
        double ham_score = (wf->spamFreq + wf->hamFreq > 0) ? wf->hamFreq / (wf->spamFreq + wf->hamFreq) : 0.0;

        if (!gtk_entry_get_text(GTK_ENTRY(fs_data->spam_count_entry))[0] || 
            (spam_count_above && wf->spamFreq >= spam_count_threshold) ||
            (!spam_count_above && wf->spamFreq <= spam_count_threshold)) {
            if (!gtk_entry_get_text(GTK_ENTRY(fs_data->ham_count_entry))[0] ||
                (ham_count_above && wf->hamFreq >= ham_count_threshold) ||
                (!ham_count_above && wf->hamFreq <= ham_count_threshold)) {
                if (!gtk_entry_get_text(GTK_ENTRY(fs_data->spam_score_entry))[0] ||
                    (spam_score_above && spam_score >= spam_score_threshold) ||
                    (!spam_score_above && spam_score <= spam_score_threshold)) {
                    if (!gtk_entry_get_text(GTK_ENTRY(fs_data->ham_score_entry))[0] ||
                        (ham_score_above && ham_score >= ham_score_threshold) ||
                        (!ham_score_above && ham_score <= ham_score_threshold)) {
                        filtered_words.emplace_back(word, *wf);
                    }
                }
            }
        }
    }

    // Sort words
    switch (sort_criterion) {
        case 0: // Alphabetical
            sort(filtered_words.begin(), filtered_words.end(),
                 [](const auto& a, const auto& b) { return a.first < b.first; });
            break;
        case 1: // Spam Count
            sort(filtered_words.begin(), filtered_words.end(),
                 [](const auto& a, const auto& b) { return a.second.spamFreq > b.second.spamFreq; });
            break;
        case 2: // Ham Count
            sort(filtered_words.begin(), filtered_words.end(),
                 [](const auto& a, const auto& b) { return a.second.hamFreq > b.second.hamFreq; });
            break;
        case 3: // Spam Score
            sort(filtered_words.begin(), filtered_words.end(),
                 [](const auto& a, const auto& b) {
                     double score_a = (a.second.spamFreq + a.second.hamFreq > 0) ?
                                      a.second.spamFreq / (a.second.spamFreq + a.second.hamFreq) : 0.0;
                     double score_b = (b.second.spamFreq + b.second.hamFreq > 0) ?
                                      b.second.spamFreq / (b.second.spamFreq + b.second.hamFreq) : 0.0;
                     return score_a > score_b;
                 });
            break;
        case 4: // Ham Score
            sort(filtered_words.begin(), filtered_words.end(),
                 [](const auto& a, const auto& b) {
                     double score_a = (a.second.spamFreq + a.second.hamFreq > 0) ?
                                      a.second.hamFreq / (a.second.spamFreq + a.second.hamFreq) : 0.0;
                     double score_b = (b.second.spamFreq + b.second.hamFreq > 0) ?
                                      b.second.hamFreq / (b.second.spamFreq + b.second.hamFreq) : 0.0;
                     return score_a > score_b;
                 });
            break;
    }

    // Clear and repopulate list store
    gtk_list_store_clear(store);
    for (const auto& pair : filtered_words) {
        const WordFreq& wf = pair.second;
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, pair.first.c_str(),
                           1, wf.spamFreq,
                           2, wf.hamFreq,
                           3, wf.spamFreq + wf.hamFreq,
                           -1);
    }
}

// Filter dialog function
void open_filter_dialog(GtkButton* button, gpointer user_data) {
    FilterSortData* fs_data = static_cast<FilterSortData*>(user_data);

    GtkWidget* filter_dialog = gtk_dialog_new_with_buttons(
        "Filter Options",
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button))),
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(filter_dialog), 400, 400);

    // Filter controls
    GtkWidget* filter_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Sort dropdown
    GtkWidget* sort_label = gtk_label_new("Sort by:");
    GtkWidget* sort_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sort_combo), "Alphabetical");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sort_combo), "Spam Count");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sort_combo), "Ham Count");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sort_combo), "Spam Score");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sort_combo), "Ham Score");
    gtk_combo_box_set_active(GTK_COMBO_BOX(sort_combo), 0);

    GtkWidget* sort_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(sort_hbox), sort_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sort_hbox), sort_combo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), sort_hbox, FALSE, FALSE, 0);

    // Alphabetical filter
    GtkWidget* alpha_label = gtk_label_new("Alphabetical Filter:");
    GtkWidget* alpha_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(alpha_entry), "Enter substring (e.g., 'free')");
    GtkWidget* alpha_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(alpha_hbox), alpha_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(alpha_hbox), alpha_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), alpha_hbox, FALSE, FALSE, 0);

    // Spam count filter
    GtkWidget* spam_count_label = gtk_label_new("Spam Count Threshold:");
    GtkWidget* spam_count_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(spam_count_entry), "e.g., 10");
    GtkWidget* spam_count_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(spam_count_combo), "Above");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(spam_count_combo), "Below");
    gtk_combo_box_set_active(GTK_COMBO_BOX(spam_count_combo), 0);
    GtkWidget* spam_count_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(spam_count_hbox), spam_count_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(spam_count_hbox), spam_count_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(spam_count_hbox), spam_count_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), spam_count_hbox, FALSE, FALSE, 0);

    // Ham count filter
    GtkWidget* ham_count_label = gtk_label_new("Ham Count Threshold:");
    GtkWidget* ham_count_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ham_count_entry), "e.g., 5");
    GtkWidget* ham_count_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ham_count_combo), "Above");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ham_count_combo), "Below");
    gtk_combo_box_set_active(GTK_COMBO_BOX(ham_count_combo), 0);
    GtkWidget* ham_count_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(ham_count_hbox), ham_count_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ham_count_hbox), ham_count_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(ham_count_hbox), ham_count_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), ham_count_hbox, FALSE, FALSE, 0);

    // Spam score filter
    GtkWidget* spam_score_label = gtk_label_new("Spam Score Threshold:");
    GtkWidget* spam_score_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(spam_score_entry), "e.g., 0.7");
    GtkWidget* spam_score_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(spam_score_combo), "Above");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(spam_score_combo), "Below");
    gtk_combo_box_set_active(GTK_COMBO_BOX(spam_score_combo), 0);
    GtkWidget* spam_score_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(spam_score_hbox), spam_score_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(spam_score_hbox), spam_score_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(spam_score_hbox), spam_score_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), spam_score_hbox, FALSE, FALSE, 0);

    // Ham score filter
    GtkWidget* ham_score_label = gtk_label_new("Ham Score Threshold:");
    GtkWidget* ham_score_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ham_score_entry), "e.g., 0.3");
    GtkWidget* ham_score_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ham_score_combo), "Above");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ham_score_combo), "Below");
    gtk_combo_box_set_active(GTK_COMBO_BOX(ham_score_combo), 0);
    GtkWidget* ham_score_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(ham_score_hbox), ham_score_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ham_score_hbox), ham_score_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(ham_score_hbox), ham_score_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), ham_score_hbox, FALSE, FALSE, 0);

    // Apply filter button
    GtkWidget* apply_button = gtk_button_new_with_label("Apply Filter");
    gtk_widget_set_name(apply_button, "apply-filter-button");
    gtk_box_pack_start(GTK_BOX(filter_box), apply_button, FALSE, FALSE, 0);

    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(filter_dialog));
    gtk_box_pack_start(GTK_BOX(content_area), filter_box, TRUE, TRUE, 0);

    // Update fs_data with new widgets
    fs_data->alpha_entry = alpha_entry;
    fs_data->spam_count_entry = spam_count_entry;
    fs_data->ham_count_entry = ham_count_entry;
    fs_data->spam_score_entry = spam_score_entry;
    fs_data->ham_score_entry = ham_score_entry;
    fs_data->spam_count_combo = spam_count_combo;
    fs_data->ham_count_combo = ham_count_combo;
    fs_data->spam_score_combo = spam_score_combo;
    fs_data->ham_score_combo = ham_score_combo;
    fs_data->sort_combo = sort_combo;

    gtk_widget_show_all(filter_dialog);

    // Connect "Apply" button to apply filter and close dialog
    g_signal_connect(apply_button, "clicked", G_CALLBACK(on_apply_filter_button_clicked), fs_data);
    g_signal_connect_swapped(apply_button, "clicked", G_CALLBACK(gtk_widget_destroy), filter_dialog);
    g_signal_connect_swapped(filter_dialog, "response", G_CALLBACK(gtk_widget_destroy), filter_dialog);
}

// Dataset viewer callback
void on_view_dataset_button_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);

    // Create dialog
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Dataset Viewer",
                                                   GTK_WINDOW(app->window),
                                                   GTK_DIALOG_MODAL,
                                                   "_Close",
                                                   GTK_RESPONSE_CLOSE,
                                                   NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);

    // Create tree view
    GtkListStore* store = gtk_list_store_new(4,
                                             G_TYPE_STRING,  // Word
                                             G_TYPE_DOUBLE,  // Spam Freq
                                             G_TYPE_DOUBLE,  // Ham Freq
                                             G_TYPE_DOUBLE); // Total Freq
    GtkWidget* tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

    // Add columns
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* column;

    column = gtk_tree_view_column_new_with_attributes("Word", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    column = gtk_tree_view_column_new_with_attributes("Spam Frequency", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    column = gtk_tree_view_column_new_with_attributes("Ham Frequency", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    column = gtk_tree_view_column_new_with_attributes("Total Frequency", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

    // Create scrolled window for tree view
    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    // Properties and Filter buttons
    GtkWidget* properties_button = gtk_button_new_with_label("Properties");
    gtk_widget_set_name(properties_button, "properties-button");
    gtk_widget_set_tooltip_text(properties_button, "View dataset properties and statistics");

    GtkWidget* filter_button = gtk_button_new_with_label("Filter");
    gtk_widget_set_tooltip_text(filter_button, "Open filter options");

    // Layout: buttons below the table
    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(button_box), properties_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), filter_button, FALSE, FALSE, 0);

    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(main_box), scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), button_box, FALSE, FALSE, 5);

    // Add to dialog content area
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content_area), main_box, TRUE, TRUE, 0);

    // Prepare filter/sort data
    FilterSortData* fs_data = new FilterSortData();
    fs_data->app = app;
    fs_data->store = store;

    // Initial population (no filter)
    fs_data->alpha_entry = gtk_entry_new();
    fs_data->spam_count_entry = gtk_entry_new();
    fs_data->ham_count_entry = gtk_entry_new();
    fs_data->spam_score_entry = gtk_entry_new();
    fs_data->ham_score_entry = gtk_entry_new();
    fs_data->spam_count_combo = gtk_combo_box_text_new();
    fs_data->ham_count_combo = gtk_combo_box_text_new();
    fs_data->spam_score_combo = gtk_combo_box_text_new();
    fs_data->ham_score_combo = gtk_combo_box_text_new();
    fs_data->sort_combo = gtk_combo_box_text_new();
    on_apply_filter_button_clicked(NULL, fs_data);

    // Connect signals
    g_signal_connect(properties_button, "clicked", G_CALLBACK(on_properties_button_clicked), app);
    g_signal_connect(filter_button, "clicked", G_CALLBACK(open_filter_dialog), fs_data);
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));

    delete fs_data;
}

// Update word frequencies based on user feedback
void updateFrequencies(AppData* app, bool isSpam) {
    for (string word : app->currentEmailWords) {
        // Normalize word
        transform(word.begin(), word.end(), word.begin(), ::tolower);
        word.erase(remove_if(word.begin(), word.end(), [](char c) { return !isalnum(c); }), word.end());
        if (word.empty()) continue;
        WordFreq* wf = app->chainMap.search(word);
        if (wf) {
            if (isSpam) {
                wf->spamFreq += 1;
            } else {
                wf->hamFreq += 1;
            }
        } else {
            WordFreq newWf(word, isSpam ? 1.0 : 0.0, isSpam ? 0.0 : 1.0);
            app->chainMap.insert(newWf);
            app->openMap.insert(newWf);
            app->wordsOrder.push_back(word);
        }
    }
    saveWordFrequenciesToTransposedCSV("/home/ka0s_5131/Desktop/Dsa_project/final.csv", app->wordsOrder, &app->chainMap);
}

// Mark as Spam button callback
void on_mark_spam_button_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    updateFrequencies(app, true);
    gtk_label_set_markup(GTK_LABEL(app->resultLabel), "<span color='#D32F2F'>Frequencies updated as Spam</span>");
}

// Mark as Ham button callback
void on_mark_ham_button_clicked(GtkButton* button, gpointer user_data) {
    AppData* app = static_cast<AppData*>(user_data);
    updateFrequencies(app, false);
    gtk_label_set_markup(GTK_LABEL(app->resultLabel), "<span color='#388E3C'>Frequencies updated as Ham</span>");
}

// Apply CSS styling
void apply_css() {
    GtkCssProvider* provider = gtk_css_provider_new();
    GError* error = NULL;
    if (!gtk_css_provider_load_from_data(provider,
        "window { background-color: #F0F0F0; }"
        ".textview text { background-color: #FFFFFF; color: #000000; font-size: 14px; }"
        ".textview { border: 1px solid #CCCCCC; padding: 10px; }"
        ".classify-button { background-color: #4CAF50; color: #FFFFFF; padding: 10px; font-weight: bold; border: none; }"
        ".classify-button:hover { background-color: #45A049; }"
        ".clear-button { background-color: #4CAF50; color: #FFFFFF; padding: 10px; font-weight: bold; border: none; }"
        ".clear-button:hover { background-color: #45A049; }"
        ".load-button { background-color: #4CAF50; color: #FFFFFF; padding: 10px; font-weight: bold; border: none; }"
        ".load-button:hover { background-color: #45A049; }"
        ".view-dataset-button { background-color: #4CAF50; color: #FFFFFF; padding: 10px; font-weight: bold; border: none; }"
        ".view-dataset-button:hover { background-color: #45A049; }"
        ".properties-button { background-color: #4CAF50; color: #FFFFFF; padding: 10px; font-weight: bold; border: none; }"
        ".properties-button:hover { background-color: #45A049; }"
        ".apply-filter-button { background-color: #4CAF50; color: #FFFFFF; padding: 10px; font-weight: bold; border: none; }"
        ".apply-filter-button:hover { background-color: #45A049; }"
        ".update-threshold-button { background-color: #4CAF50; color: #FFFFFF; padding: 10px; font-weight: bold; border: none; }"
        ".update-threshold-button:hover { background-color: #45A049; }"
        ".mark-spam-button { background-color: #FF5252; color: #FFFFFF; padding: 10px; font-weight: bold; border: none; }"
        ".mark-spam-button:hover { background-color: #E04848; }"
        ".mark-ham-button { background-color: #2196F3; color: #FFFFFF; padding: 10px; font-weight: bold; border: none; }"
        ".mark-ham-button:hover { background-color: #1E88E5; }"
        ".result-label { font-weight: bold; font-size: 16px; margin: 10px; }"
        "box { padding: 10px; }",
        -1, &error)) {
        cerr << "CSS Error: " << error->message << endl;
        g_error_free(error);
    }

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// Main function
int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    // Apply CSS styling
    apply_css();

    AppData app;
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Email Classification");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 800, 600);
    gtk_container_set_border_width(GTK_CONTAINER(app.window), 10);
    gtk_widget_set_name(app.window, "window");
    app.spamThreshold = 0.7; // Initialize threshold

    app.textView = gtk_text_view_new();
    gtk_widget_set_size_request(app.textView, 600, 400);
    gtk_widget_set_name(app.textView, "textview");
    gtk_widget_set_tooltip_text(app.textView, "Enter or load email text here");

    app.classifyButton = gtk_button_new_with_label("Classify");
    gtk_widget_set_name(app.classifyButton, "classify-button");
    gtk_widget_set_tooltip_text(app.classifyButton, "Classify the email content");

    app.clearButton = gtk_button_new_with_label("Clear Screen");
    gtk_widget_set_name(app.clearButton, "clear-button");
    gtk_widget_set_tooltip_text(app.clearButton, "Clear the text and results");

    app.loadButton = gtk_button_new_with_label("Load Email");
    gtk_widget_set_name(app.loadButton, "load-button");
    gtk_widget_set_tooltip_text(app.loadButton, "Load email from a text file");

    app.viewDatasetButton = gtk_button_new_with_label("View Dataset");
    gtk_widget_set_name(app.viewDatasetButton, "view-dataset-button");
    gtk_widget_set_tooltip_text(app.viewDatasetButton, "View word frequencies in the dataset");

    app.resultLabel = gtk_label_new("");
    gtk_widget_set_name(app.resultLabel, "result-label");
    gtk_widget_set_tooltip_text(app.resultLabel, "Shows classification result");

    app.markSpamButton = gtk_button_new_with_label("Mark as Spam");
    gtk_widget_set_name(app.markSpamButton, "mark-spam-button");
    gtk_widget_set_tooltip_text(app.markSpamButton, "Mark email as spam and update frequencies");

    app.markHamButton = gtk_button_new_with_label("Mark as Ham");
    gtk_widget_set_name(app.markHamButton, "mark-ham-button");
    gtk_widget_set_tooltip_text(app.markHamButton, "Mark email as ham and update frequencies");

    gtk_widget_set_sensitive(app.markSpamButton, FALSE);
    gtk_widget_set_sensitive(app.markHamButton, FALSE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(box), app.textView, TRUE, TRUE, 0);

    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(buttonBox), app.classifyButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), app.clearButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), app.loadButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), app.viewDatasetButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), buttonBox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), app.resultLabel, FALSE, FALSE, 0);

    GtkWidget* markBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(markBox), app.markSpamButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(markBox), app.markHamButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), markBox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(app.window), box);

    // Create highlighting tags
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.textView));
    for (int i = 1; i <= 5; ++i) {
        string tagName = "spam-" + to_string(i);
        gtk_text_buffer_create_tag(buffer, tagName.c_str(), "foreground", get_spam_color(i), NULL);
        tagName = "ham-" + to_string(i);
        gtk_text_buffer_create_tag(buffer, tagName.c_str(), "foreground", get_ham_color(i), NULL);
    }

    // Load word frequencies once at startup
    loadWordFrequenciesFromTransposedCSV("/home/ka0s_5131/Desktop/Dsa_project/final.csv", &app.chainMap, &app.openMap, app.wordsOrder);

    // Connect signals
    g_signal_connect(app.classifyButton, "clicked", G_CALLBACK(on_classify_button_clicked), &app);
    g_signal_connect(app.clearButton, "clicked", G_CALLBACK(on_clear_button_clicked), &app);
    g_signal_connect(app.loadButton, "clicked", G_CALLBACK(on_load_button_clicked), &app);
    g_signal_connect(app.viewDatasetButton, "clicked", G_CALLBACK(on_view_dataset_button_clicked), &app);
    g_signal_connect(app.markSpamButton, "clicked", G_CALLBACK(on_mark_spam_button_clicked), &app);
    g_signal_connect(app.markHamButton, "clicked", G_CALLBACK(on_mark_ham_button_clicked), &app);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(app.window);
    gtk_main();
    return 0;
}
