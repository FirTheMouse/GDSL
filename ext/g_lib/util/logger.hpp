#pragma once

#include "../util/util.hpp"
#include "../core/object.hpp"
#include "../util/strings.hpp"
#include <chrono>
#include <iostream>


namespace Log {

// Provides the time it takes for a function to run, not avereged over iterations
inline double time_function(int ITERATIONS,std::function<void(int)> process) {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        process(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return (double)time.count();
}

struct Comparison_ {
    Comparison_() {}
    Comparison_(int a, int b, int c, int d) : a_table(a), a_row(b), b_table(c), b_row(d) {}
    int a_table, a_row;
    int b_table, b_row;
};


inline void run_rig(list<list<std::function<void(int)>>> f_table,list<list<std::string>> s_table,list<Comparison_> comps,bool warm_up,int PROCESS_ITERATIONS,int C_ITS) {
    list<list<double>> t_table;

    for(int c=0;c<f_table.length();c++) {
        t_table.push(list<double>{});
        for(int r=0;r<f_table[c].length();r++) {
            t_table[c].push(0.0);
        }
    }

    for(int m = 0;m<(warm_up?2:1);m++) {
        int C_ITERATIONS = m==0?warm_up?1:C_ITS:C_ITS;

        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]=0.0;
            }
        }

        for(int i = 0;i<C_ITERATIONS;i++)
        {
            for(int c=0;c<f_table.length();c++) {
                for(int r=0;r<f_table[c].length();r++) {
                    // if(r==0) print("Running: ",s_table[c][r]);
                    double time = time_function(PROCESS_ITERATIONS,f_table[c][r]);
                    t_table[c][r]+=time;
                }
            }
        }
        if(warm_up) {
        print("-------------------------");
        print(m==0 ? "      ==COLD==" : "       ==WARM==");
        }
        print("-------------------------");
        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]/=C_ITERATIONS;
                print(s_table[c][r],": ",t_table[c][r]," ns (",t_table[c][r] / PROCESS_ITERATIONS," ns per operation)");
            }
            print("-------------------------");
        }
        for(auto v : comps) {
            double factor = t_table[v.a_table][v.a_row]/t_table[v.b_table][v.b_row];
            std::string sfs;
            double tolerance = 5.0;
            if (std::abs(factor - 1.0) < tolerance/100.0) {
                sfs = "around the same as ";
            } else if (factor > 1.0) {
                double percentage = (factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% slower than ";
            } else {
                double percentage = (1.0/factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% faster than ";
            }
            print("Factor [",s_table[v.a_table][v.a_row],"/",s_table[v.b_table][v.b_row],
            "]: ",factor," (",s_table[v.a_table][v.a_row]," is ",sfs,s_table[v.b_table][v.b_row],")");
        }
        print("-------------------------");

    }
}

// A helper for using the benchmarking tools to reduce boilerplate
struct rig {
private:

    list<list<std::function<void(int)>>> f_table;
    list<list<std::string>> s_table;
    list<Comparison_> comps;
    map<std::string,std::pair<int,int>> processes;
public:
    //Adds another table, tables are isolated test blocks
    void add_table() {
        f_table << list<std::function<void(int)>>{};
        s_table << list<std::string>{};
    }

    /// @brief Add a process to run
    /// @param process_name Name of the process, used for lookup and display when run
    /// @param process The function to time and run, the int argument is the process iteration, assuming PROCESS_ITERATIONS is not 1
    /// @param table Default value is 0, this can be used to split processes into distinct blocks when run
    void add_process(const std::string& process_name,std::function<void(int)> process,int table = 0) {
        while(f_table.length() <= table) add_table();
        processes.put(process_name,std::make_pair(table,f_table.get(table).length()));
        s_table.get(table) << process_name;
        f_table.get(table) << process;
    }

    /// @brief Add a comparison to be printed
    /// @param a Process to compare against
    /// @param b Process to compare to a
    void add_comparison(const std::string& a,const std::string& b) {
        try {
            std::pair<int,int> ap = processes.get(a);
            std::pair<int,int> bp = processes.get(b);
            comps << Comparison_(ap.first,ap.second,bp.first,bp.second);
        } catch(std::exception e) {
            if(!processes.hasKey(a)) {
                print("rig::110 Unable to add comparison to rig: ",a," was never added as a process");
            }
            if(!processes.hasKey(b)) {
                print("rig::110 Unable to add comparison to rig: ",b," was never added as a process");
            }
        }
    }

    /// @brief Run the rig and print out the results of the benchmark
    /// @param C_ITS How many iterations of the processes there should be, this contributes to the averege
    /// @param warm_up Whether or not to do a cold run to warm up the cache
    /// @param PROCESS_ITERATIONS How many times each process should run, not part of the averege
    void run(int C_ITS,bool warm_up = false,int PROCESS_ITERATIONS = 1) {
        run_rig(f_table,s_table,comps,warm_up,PROCESS_ITERATIONS,C_ITS);
    }
};


struct SeqLine : public q_object
{
    SeqLine() {};
    SeqLine(const std::string _label, bool _is_log) {
        label = _label;
        is_log = _is_log;
        Log::Line new_timer; new_timer.start();
        timer = new_timer;
    }

    Log::Line timer;
    std::string label = "";
    SeqLine* parent = nullptr;
    list<g_ptr<SeqLine>> children;
    bool is_log = true;

    std::string get_indent() {
        if(!parent) return "";
        int depth = 0;
        SeqLine* cursor = this;
        while(cursor->parent) { depth++; cursor = cursor->parent; }
        std::string indent(depth * 3, ' ');
        return indent;
    }

    std::string to_string() {
        std::string indent = get_indent();
        std::string to_return = "";
        if(is_log) {
            to_return.append(indent+label+"\n");
        } else {
            to_return.append(indent+label+" [time: " + ftime(timer.total_time_)+"]\n");
            for(auto& child : children) {
                to_return.append(child->to_string());
            }
        }
        return to_return;
    }
};

class Span : public Object
{
public:
    Span() {
        line_root = make<SeqLine>("Root",false);
    };

    map<std::string, Log::Line> timers;
    map<std::string, int> counters;
    bool print_on_line_end = false;
    bool log_everything = false;

    void start_timer(const std::string &label)
    {
        if (timers.hasKey(label))
        {
            Log::Line &timer = timers.get(label);
            timer.start();
        }
        else
        {
            Log::Line timer;
            timer.start();
            timers.put(label, timer);
        }
    }

    double end_timer(const std::string &label)
    {
        if (timers.hasKey(label))
        {
            Log::Line &timer = timers.get(label);
            return timer.end();
        }
        return 0.0;
    }

    double get_time(const std::string &label)
    {
        if (timers.hasKey(label))
        {
            return timers.get(label).total_time_;
        }
        else
        {
            return -1.0;
        }
    }

    std::string timer_string(const std::string &label)
    {
        return label + ": " + ftime(get_time(label));
    }

    void print_timers()
    {
        for (auto label : timers.keySet())
        {
            print(timer_string(label));
        }
    }

    void increment(const std::string &label, int by = 1)
    {
        counters.getOrPut(label, 0) += by;
    }

    int get_count(const std::string &label)
    {
        return counters.getOrDefault(label, 0);
    }

    void print_counters()
    {
        for (auto label : counters.keySet())
        {
            print(label, ": ", get_count(label));
        }
    }

    g_ptr<SeqLine> line_root = nullptr;
    g_ptr<SeqLine> on_line = nullptr;

    g_ptr<SeqLine> get_last_line() {
        if(on_line) return on_line;
        else return line_root;
    }

    void add_line(const std::string& label) {
        g_ptr<SeqLine> parent = get_last_line();
        parent->children << make<SeqLine>(label,false);
        parent->children.last()->parent = parent.getPtr();
        on_line = parent->children.last();

        if(log_everything)
            print(label);
    }

    double end_line() 
    {
        double time = 0.0;
        if(!on_line) return time;
        time = on_line->timer.end();
        if(print_on_line_end)
            std::cout << on_line->to_string() << std::flush;
        if(on_line->parent) {
            on_line = on_line->parent;
        }
        return time;
    }

    template<typename... Args>
    void log(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        if(log_everything)
            std::cout << oss.str() << std::endl;
        std::string indent = get_last_line()->get_indent();
        indent += "  > "; //Extra space to distinquish from header
        std::string msg = indent+oss.str();
        indent_multiline(msg,indent);
        g_ptr<SeqLine> new_log = make<SeqLine>(msg,true);
        get_last_line()->children << new_log;
    }

    void print_all() {
        line_root->timer.end();
        print(line_root->to_string());
    }

    void newline(const std::string& label) {
        add_line(label);
    }

    double endline() {
        return end_line();
    }
};

}



    