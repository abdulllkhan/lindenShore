#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

// Include the classes from pnl_calculator_main.cpp (without main function)
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
        ifstream file(filename.c_str());
        
        if (!file.is_open()) {
            return trades;
        }
        
        string line;
        getline(file, line); // Skip header
        
        while (getline(file, line)) {
            if (line.empty()) continue;
            
            vector<string> tokens = split(line, ',');
            if (tokens.size() >= 5) {
                long timestamp = strtol(tokens[0].c_str(), NULL, 10);
                string symbol = tokens[1];
                char side = tokens[2][0];
                double price = strtod(tokens[3].c_str(), NULL);
                long quantity = strtol(tokens[4].c_str(), NULL, 10);
                
                trades.push_back(Trade(timestamp, symbol, side, price, quantity));
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

// Helper function to create test CSV files
void createTestFile(const string& filename, const string& content) {
    ofstream file(filename.c_str());
    file << content;
    file.close();
}

// Test Basic FIFO vs LIFO
TEST(PnLCalculatorTest, BasicFIFOvsLIFO) {
    // Create test trades: Buy 10@10, Buy 10@20, Sell 10@15
    vector<Trade> trades;
    trades.push_back(Trade(101, "TEST", 'B', 10.0, 10));
    trades.push_back(Trade(102, "TEST", 'B', 20.0, 10));
    trades.push_back(Trade(103, "TEST", 'S', 15.0, 10));
    
    // Test FIFO
    PnLCalculator fifoCalculator(PnLCalculator::FIFO);
    vector<PnLResult> fifoResults = fifoCalculator.processTrades(trades);
    
    ASSERT_EQ(fifoResults.size(), 1);
    EXPECT_EQ(fifoResults[0].timestamp, 103);
    EXPECT_EQ(fifoResults[0].symbol, "TEST");
    EXPECT_DOUBLE_EQ(fifoResults[0].pnl, 50.0); // (15-10)*10 = 50
    
    // Test LIFO
    PnLCalculator lifoCalculator(PnLCalculator::LIFO);
    vector<PnLResult> lifoResults = lifoCalculator.processTrades(trades);
    
    ASSERT_EQ(lifoResults.size(), 1);
    EXPECT_EQ(lifoResults[0].timestamp, 103);
    EXPECT_EQ(lifoResults[0].symbol, "TEST");
    EXPECT_DOUBLE_EQ(lifoResults[0].pnl, -50.0); // (15-20)*10 = -50
}

// Test Zero PnL
TEST(PnLCalculatorTest, ZeroPnL) {
    vector<Trade> trades;
    trades.push_back(Trade(101, "ZERO", 'B', 100.0, 10));
    trades.push_back(Trade(102, "ZERO", 'S', 100.0, 10));
    
    PnLCalculator calculator(PnLCalculator::FIFO);
    vector<PnLResult> results = calculator.processTrades(trades);
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_DOUBLE_EQ(results[0].pnl, 0.0);
}

// Test Short Selling
TEST(PnLCalculatorTest, ShortSelling) {
    vector<Trade> trades;
    trades.push_back(Trade(101, "SHORT", 'S', 50.0, 10)); // Short 10@50
    trades.push_back(Trade(102, "SHORT", 'B', 45.0, 10)); // Cover at 45
    
    PnLCalculator calculator(PnLCalculator::FIFO);
    vector<PnLResult> results = calculator.processTrades(trades);
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_DOUBLE_EQ(results[0].pnl, 50.0); // (50-45)*10 = 50 profit from short
}

// Test Multi-Symbol
TEST(PnLCalculatorTest, MultiSymbol) {
    vector<Trade> trades;
    trades.push_back(Trade(101, "AAPL", 'B', 100.0, 10));
    trades.push_back(Trade(102, "GOOGL", 'B', 2000.0, 5));
    trades.push_back(Trade(103, "AAPL", 'S', 110.0, 5));
    trades.push_back(Trade(104, "GOOGL", 'S', 2100.0, 3));
    
    PnLCalculator calculator(PnLCalculator::FIFO);
    vector<PnLResult> results = calculator.processTrades(trades);
    
    ASSERT_EQ(results.size(), 2);
    
    // AAPL trade
    EXPECT_EQ(results[0].symbol, "AAPL");
    EXPECT_DOUBLE_EQ(results[0].pnl, 50.0); // (110-100)*5
    
    // GOOGL trade  
    EXPECT_EQ(results[1].symbol, "GOOGL");
    EXPECT_DOUBLE_EQ(results[1].pnl, 300.0); // (2100-2000)*3
}

// Test Large Quantities
TEST(PnLCalculatorTest, LargeQuantities) {
    vector<Trade> trades;
    trades.push_back(Trade(101, "BIG", 'B', 10.0, 1000000000L)); // 1 billion
    trades.push_back(Trade(102, "BIG", 'S', 11.0, 1000000000L));
    
    PnLCalculator calculator(PnLCalculator::FIFO);
    vector<PnLResult> results = calculator.processTrades(trades);
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_DOUBLE_EQ(results[0].pnl, 1000000000.0); // (11-10)*1B = 1B
}

// Test Partial Fills
TEST(PnLCalculatorTest, PartialFills) {
    vector<Trade> trades;
    trades.push_back(Trade(101, "PARTIAL", 'B', 100.0, 15)); // Buy 15
    trades.push_back(Trade(102, "PARTIAL", 'B', 200.0, 15)); // Buy 15 more
    trades.push_back(Trade(103, "PARTIAL", 'S', 150.0, 20)); // Sell 20 (partial fill)
    trades.push_back(Trade(104, "PARTIAL", 'S', 175.0, 10)); // Sell remaining 10
    
    PnLCalculator calculator(PnLCalculator::FIFO);
    vector<PnLResult> results = calculator.processTrades(trades);
    
    ASSERT_EQ(results.size(), 2);
    
    // First sell: 15*(150-100) + 5*(150-200) = 750 + (-250) = 500
    EXPECT_DOUBLE_EQ(results[0].pnl, 500.0);
    
    // Second sell: 10*(175-200) = -250
    EXPECT_DOUBLE_EQ(results[1].pnl, -250.0);
}

// Test CSV Parser
TEST(CSVParserTest, ParseValidFile) {
    // Create test CSV file
    string testContent = 
        "TIMESTAMP,SYMBOL,BUY_OR_SELL,PRICE,QUANTITY\n"
        "101,TEST,B,10.50,100\n"
        "102,TEST,S,11.00,50\n";
    
    createTestFile("test_parse.csv", testContent);
    
    vector<Trade> trades = CSVParser::parseFile("test_parse.csv");
    
    ASSERT_EQ(trades.size(), 2);
    
    EXPECT_EQ(trades[0].getTimestamp(), 101);
    EXPECT_EQ(trades[0].getSymbol(), "TEST");
    EXPECT_EQ(trades[0].getSide(), 'B');
    EXPECT_DOUBLE_EQ(trades[0].getPrice(), 10.5);
    EXPECT_EQ(trades[0].getQuantity(), 100);
    
    EXPECT_EQ(trades[1].getTimestamp(), 102);
    EXPECT_EQ(trades[1].getSymbol(), "TEST");
    EXPECT_EQ(trades[1].getSide(), 'S');
    EXPECT_DOUBLE_EQ(trades[1].getPrice(), 11.0);
    EXPECT_EQ(trades[1].getQuantity(), 50);
    
    // Cleanup
    remove("test_parse.csv");
}

// Test Empty File
TEST(CSVParserTest, ParseEmptyFile) {
    createTestFile("test_empty.csv", "TIMESTAMP,SYMBOL,BUY_OR_SELL,PRICE,QUANTITY\n");
    
    vector<Trade> trades = CSVParser::parseFile("test_empty.csv");
    EXPECT_EQ(trades.size(), 0);
    
    remove("test_empty.csv");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}