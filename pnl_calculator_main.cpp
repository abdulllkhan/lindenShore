#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <cctype>

using namespace std;

class Trade {
public:
    Trade(long timestamp, const string& symbol, char side, double price, long quantity)
        : timestamp_(timestamp), symbol_(symbol), side_(side), price_(price), quantity_(quantity) {}
    
    long getTimestamp() const { return timestamp_; }
    const string& getSymbol() const { return symbol_; }
    char getSide() const { return side_; }
    double getPrice() const { return price_; }
    long getQuantity() const { return quantity_; }
    
private:
    long timestamp_;
    string symbol_;
    char side_;
    double price_;
    long quantity_;
};

class Position {
public:
    Position(double price, long quantity)
        : price_(price), quantity_(quantity) {}
    
    double getPrice() const { return price_; }
    long getQuantity() const { return quantity_; }
    void setQuantity(long quantity) { quantity_ = quantity; }
    
private:
    double price_;
    long quantity_;
};

struct PnLResult {
    long timestamp;
    string symbol;
    double pnl;
};

class PnLCalculator {
public:
    enum AccountingScheme {
        FIFO,
        LIFO
    };
    
    PnLCalculator(AccountingScheme scheme) : scheme_(scheme) {}
    
    vector<PnLResult> processTrades(const vector<Trade>& trades) {
        vector<PnLResult> results;
        
        for (vector<Trade>::const_iterator it = trades.begin(); it != trades.end(); ++it) {
            const Trade& trade = *it;
            const string& symbol = trade.getSymbol();
            char side = trade.getSide();
            
            if (!positions_.count(symbol)) {
                positions_[symbol] = deque<Position>();
            }
            
            deque<Position>& symbolPositions = positions_[symbol];
            
            bool hasOpenPositions = false;
            if (!symbolPositions.empty()) {
                long positionQuantity;
                if (scheme_ == FIFO) {
                    positionQuantity = symbolPositions.front().getQuantity();
                } else { // LIFO
                    positionQuantity = symbolPositions.front().getQuantity(); // LIFO also uses front()
                }
                
                if ((side == 'B' && positionQuantity < 0) ||
                    (side == 'S' && positionQuantity > 0)) {
                    hasOpenPositions = true;
                }
            }
            
            if (hasOpenPositions) {
                PnLResult result = clearPositions(trade);
                results.push_back(result);
            } else {
                long quantity = (side == 'B') ? trade.getQuantity() : -trade.getQuantity();
                if (scheme_ == LIFO) {
                    symbolPositions.push_front(Position(trade.getPrice(), quantity));
                } else {
                    symbolPositions.push_back(Position(trade.getPrice(), quantity));
                }
            }
        }
        
        return results;
    }
    
private:
    AccountingScheme scheme_;
    map<string, deque<Position> > positions_;
    
    PnLResult clearPositions(const Trade& trade) {
        PnLResult result;
        result.timestamp = trade.getTimestamp();
        result.symbol = trade.getSymbol();
        result.pnl = 0.0;
        
        deque<Position>& symbolPositions = positions_[trade.getSymbol()];
        long remainingQuantity = trade.getQuantity();
        char side = trade.getSide();
        
        while (remainingQuantity > 0 && !symbolPositions.empty()) {
            Position* position;
            
            if (scheme_ == FIFO) {
                position = &symbolPositions.front();
            } else { // LIFO
                position = &symbolPositions.front(); // LIFO uses front() because we push_front()
            }
            
            long positionQuantity = abs(position->getQuantity());
            long clearedQuantity = min(remainingQuantity, positionQuantity);
            
            double pnl = calculatePnL(trade, *position, clearedQuantity);
            result.pnl += pnl;
            
            long newQuantity = positionQuantity - clearedQuantity;
            if (newQuantity == 0) {
                if (scheme_ == FIFO) {
                    symbolPositions.pop_front();
                } else { // LIFO
                    symbolPositions.pop_front(); // LIFO pops from front too
                }
            } else {
                // Partial clear
                long sign = (position->getQuantity() > 0) ? 1 : -1;
                position->setQuantity(sign * newQuantity);
            }
            
            remainingQuantity -= clearedQuantity;
        }
        
        if (remainingQuantity > 0) {
            long quantity = (side == 'B') ? remainingQuantity : -remainingQuantity;
            if (scheme_ == LIFO) {
                symbolPositions.push_front(Position(trade.getPrice(), quantity));
            } else {
                symbolPositions.push_back(Position(trade.getPrice(), quantity));
            }
        }
        
        return result;
    }
    
    double calculatePnL(const Trade& trade, Position& position, long quantity) {
        double pnl = 0.0;
        
        if (trade.getSide() == 'S') {
            pnl = quantity * (trade.getPrice() - position.getPrice());
        } else {
            // closing short positions
            pnl = quantity * (position.getPrice() - trade.getPrice());
        }
        
        return pnl;
    }
};

class CSVParser {
public:
    static vector<Trade> parseFile(const string& filename) {
        vector<Trade> trades;
        ifstream file(filename);
        
        if (!file.is_open()) {
            cerr << "Error: Could not open file " << filename << endl;
            return trades;
        }
        
        string line;
        getline(file, line); 
        
        // handling malformed header
        for (size_t i = 0; i < line.length(); ++i) {
            if (line[i] == ' ') {
                line[i] = '\n';
            }
        }
        
        stringstream ss(line);
        string headerLine;
        getline(ss, headerLine); 
        
        string tradeLine;
        while (getline(ss, tradeLine)) {
            if (tradeLine.empty()) continue;
            
            vector<string> tokens = split(tradeLine, ',');
            if (tokens.size() >= 5) {
                long timestamp = strtol(tokens[0].c_str(), NULL, 10);
                string symbol = tokens[1];
                char side = tokens[2][0];
                double price = strtod(tokens[3].c_str(), NULL);
                long quantity = strtol(tokens[4].c_str(), NULL, 10);
                trades.push_back(Trade(timestamp, symbol, side, price, quantity));
            }
        }
        
        while (getline(file, line)) {
            if (line.empty()) continue;
            
            vector<string> tokens = split(line, ',');
            if (tokens.size() >= 5) {
                long timestamp = stol(tokens[0]);
                string symbol = tokens[1];
                char side = tokens[2][0];
                double price = stod(tokens[3]);
                long quantity = stol(tokens[4]);
                
                trades.emplace_back(timestamp, symbol, side, price, quantity);
            }
        }
        
        return trades;
    }
    
private:
    static vector<string> split(const string& str, char delimiter) {
        vector<string> tokens;
        stringstream ss(str);
        string token;
        
        while (getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <csv_file> <fifo|lifo>" << endl;
        return 1;
    }
    
    string filename = argv[1];
    string method = argv[2];
    
    PnLCalculator::AccountingScheme scheme;
    if (method == "fifo") {
        scheme = PnLCalculator::FIFO;
    } else if (method == "lifo") {
        scheme = PnLCalculator::LIFO;
    } else {
        cerr << "Error: Method must be 'fifo' or 'lifo'" << endl;
        return 1;
    }
    
    vector<Trade> trades = CSVParser::parseFile(filename);
    if (trades.empty()) {
        cerr << "Error: No trades found in file" << endl;
        return 1;
    }
    
    PnLCalculator calculator(scheme);
    vector<PnLResult> results = calculator.processTrades(trades);

    cout << "TIMESTAMP,SYMBOL,PNL" << endl;

    for (vector<PnLResult>::const_iterator it = results.begin(); it != results.end(); ++it) {
        const PnLResult& result = *it;
        double displayPnl = (fabs(result.pnl) < 1e-9) ? 0.0 : result.pnl;
        cout << result.timestamp << "," << result.symbol << "," 
             << fixed << setprecision(2) << displayPnl << endl;
    }
    
    return 0;
}