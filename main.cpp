
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

enum class SubmissionStatus {
    Accepted,
    Wrong_Answer,
    Runtime_Error,
    Time_Limit_Exceed
};

struct Submission {
    string problem_name;
    string team_name;
    SubmissionStatus status;
    int time;
    
    Submission(const string& p, const string& t, SubmissionStatus s, int tm)
        : problem_name(p), team_name(t), status(s), time(tm) {}
};

struct ProblemSubmission {
    vector<Submission> submissions;
    bool solved = false;
    int solve_time = -1;
    int wrong_attempts_before_solve = 0;
    int submissions_after_freeze = 0;
    bool is_frozen = false;
    int freeze_time = -1;
    
    void add_submission(const Submission& sub, bool is_frozen_period) {
        submissions.push_back(sub);
        
        if (is_frozen_period && !solved) {
            submissions_after_freeze++;
        }
        
        if (!solved && sub.status == SubmissionStatus::Accepted) {
            solved = true;
            solve_time = sub.time;
            wrong_attempts_before_solve = 0;
            for (const auto& prev_sub : submissions) {
                if (&prev_sub != &sub && prev_sub.status != SubmissionStatus::Accepted) {
                    wrong_attempts_before_solve++;
                }
            }
        }
    }
    
    void set_frozen(bool frozen, int f_time) {
        if (!solved) {
            bool has_submission_after_freeze = false;
            for (const auto& sub : submissions) {
                if (sub.time >= f_time) {
                    has_submission_after_freeze = true;
                    break;
                }
            }
            is_frozen = frozen && has_submission_after_freeze;
            if (is_frozen) {
                freeze_time = f_time;
            }
        }
    }
    
    int get_penalty_time() const {
        if (!solved) return 0;
        return 20 * wrong_attempts_before_solve + solve_time;
    }
    
    string get_status_display() const {
        if (is_frozen) {
            int wrong_before = 0;
            int submissions_after = 0;
            for (const auto& sub : submissions) {
                if (sub.status != SubmissionStatus::Accepted) {
                    wrong_before++;
                } else {
                    break;
                }
            }
            for (const auto& sub : submissions) {
                if (sub.time >= freeze_time) {
                    submissions_after++;
                }
            }
            if (wrong_before == 0) {
                return "0/" + to_string(submissions_after);
            } else {
                return to_string(wrong_before) + "/" + to_string(submissions_after);
            }
        } else if (solved) {
            if (wrong_attempts_before_solve == 0) {
                return "+";
            } else {
                return "+" + to_string(wrong_attempts_before_solve);
            }
        } else {
            int wrong_count = 0;
            for (const auto& sub : submissions) {
                if (sub.status != SubmissionStatus::Accepted) {
                    wrong_count++;
                }
            }
            if (wrong_count == 0) {
                return ".";
            } else {
                return "-" + to_string(wrong_count);
            }
        }
    }
};

struct Team {
    string name;
    map<char, ProblemSubmission> problems;
    
    Team() = default;
    Team(const string& n) : name(n) {}
    
    void add_submission(const Submission& sub, bool is_frozen_period) {
        problems[sub.problem_name[0]].add_submission(sub, is_frozen_period);
    }
    
    int get_solved_count() const {
        int count = 0;
        for (const auto& pair : problems) {
            if (pair.second.solved && !pair.second.is_frozen) {
                count++;
            }
        }
        return count;
    }
    
    int get_total_penalty() const {
        int total = 0;
        for (const auto& pair : problems) {
            if (pair.second.solved && !pair.second.is_frozen) {
                total += pair.second.get_penalty_time();
            }
        }
        return total;
    }
    
    vector<int> get_solve_times() const {
        vector<int> times;
        for (const auto& pair : problems) {
            if (pair.second.solved && !pair.second.is_frozen) {
                times.push_back(pair.second.solve_time);
            }
        }
        sort(times.rbegin(), times.rend());
        return times;
    }
};

struct Competition {
    bool started = false;
    bool ended = false;
    int duration = 0;
    int problem_count = 0;
    int freeze_time = -1;
    bool is_frozen = false;
    map<string, Team> teams;
    vector<Submission> all_submissions;
    bool scoreboard_flushed = false;
    
    bool add_team(const string& name) {
        if (started) return false;
        if (teams.find(name) != teams.end()) return false;
        teams[name] = Team(name);
        return true;
    }
    
    bool start_competition(int dur, int prob_count) {
        if (started) return false;
        started = true;
        duration = dur;
        problem_count = prob_count;
        return true;
    }
    
    void add_submission(const string& problem_name, const string& team_name, 
                       SubmissionStatus status, int time) {
        Submission sub(problem_name, team_name, status, time);
        all_submissions.push_back(sub);
        
        if (teams.find(team_name) != teams.end()) {
            bool is_frozen_period = is_frozen && time >= freeze_time;
            teams[team_name].add_submission(sub, is_frozen_period);
        }
    }
    
    void flush_scoreboard() {
        scoreboard_flushed = true;
    }
    
    bool freeze_scoreboard() {
        if (is_frozen) return false;
        is_frozen = true;
        if (!all_submissions.empty()) {
            freeze_time = all_submissions.back().time;
            
            // Set frozen status for all unsolved problems that have submissions after freeze time
            for (auto& team_pair : teams) {
                Team& team = team_pair.second;
                for (auto& prob_pair : team.problems) {
                    ProblemSubmission& prob_sub = prob_pair.second;
                    prob_sub.set_frozen(true, freeze_time);
                }
            }
        }
        return true;
    }
    
    void unfreeze_problem(const string& team_name, char problem) {
        if (teams.find(team_name) != teams.end()) {
            auto& prob_sub = teams[team_name].problems[problem];
            if (prob_sub.is_frozen) {
                prob_sub.is_frozen = false;
                prob_sub.submissions_after_freeze = 0;
            }
        }
    }
    
    vector<pair<string, Team>> get_ranked_teams() const {
        vector<pair<string, Team>> ranked_teams;
        for (const auto& pair : teams) {
            ranked_teams.push_back(pair);
        }
        
        sort(ranked_teams.begin(), ranked_teams.end(), 
             [](const pair<string, Team>& a, const pair<string, Team>& b) {
                 const Team& team_a = a.second;
                 const Team& team_b = b.second;
                 
                 int solved_a = team_a.get_solved_count();
                 int solved_b = team_b.get_solved_count();
                 
                 if (solved_a != solved_b) {
                     return solved_a > solved_b;
                 }
                 
                 int penalty_a = team_a.get_total_penalty();
                 int penalty_b = team_b.get_total_penalty();
                 
                 if (penalty_a != penalty_b) {
                     return penalty_a < penalty_b;
                 }
                 
                 vector<int> times_a = team_a.get_solve_times();
                 vector<int> times_b = team_b.get_solve_times();
                 
                 size_t min_size = min(times_a.size(), times_b.size());
                 for (size_t i = 0; i < min_size; i++) {
                     if (times_a[i] != times_b[i]) {
                         return times_a[i] < times_b[i];
                     }
                 }
                 
                 return team_a.name < team_b.name;
             });
        
        return ranked_teams;
    }
    
    int get_team_ranking(const string& team_name) const {
        if (teams.find(team_name) == teams.end()) return -1;
        
        auto ranked_teams = get_ranked_teams();
        for (size_t i = 0; i < ranked_teams.size(); i++) {
            if (ranked_teams[i].first == team_name) {
                return i + 1;
            }
        }
        return -1;
    }
    
    Submission* find_last_submission(const string& team_name, const string& problem, 
                                   const string& status) {
        if (teams.find(team_name) == teams.end()) return nullptr;
        
        Submission* result = nullptr;
        for (auto& sub : all_submissions) {
            if (sub.team_name == team_name) {
                bool problem_match = (problem == "ALL" || sub.problem_name == problem);
                bool status_match = (status == "ALL" || 
                    (status == "Accepted" && sub.status == SubmissionStatus::Accepted) ||
                    (status == "Wrong_Answer" && sub.status == SubmissionStatus::Wrong_Answer) ||
                    (status == "Runtime_Error" && sub.status == SubmissionStatus::Runtime_Error) ||
                    (status == "Time_Limit_Exceed" && sub.status == SubmissionStatus::Time_Limit_Exceed));
                
                if (problem_match && status_match) {
                    result = &sub;
                }
            }
        }
        return result;
    }
};

SubmissionStatus parse_status(const string& status_str) {
    if (status_str == "Accepted") return SubmissionStatus::Accepted;
    if (status_str == "Wrong_Answer") return SubmissionStatus::Wrong_Answer;
    if (status_str == "Runtime_Error") return SubmissionStatus::Runtime_Error;
    if (status_str == "Time_Limit_Exceed") return SubmissionStatus::Time_Limit_Exceed;
    return SubmissionStatus::Wrong_Answer;
}

string status_to_string(SubmissionStatus status) {
    switch (status) {
        case SubmissionStatus::Accepted: return "Accepted";
        case SubmissionStatus::Wrong_Answer: return "Wrong_Answer";
        case SubmissionStatus::Runtime_Error: return "Runtime_Error";
        case SubmissionStatus::Time_Limit_Exceed: return "Time_Limit_Exceed";
    }
    return "Unknown";
}

void print_scoreboard(const Competition& comp) {
    auto ranked_teams = comp.get_ranked_teams();
    
    for (size_t i = 0; i < ranked_teams.size(); i++) {
        const auto& team_pair = ranked_teams[i];
        const Team& team = team_pair.second;
        
        cout << team.name << " " << (i + 1) << " " << team.get_solved_count() 
             << " " << team.get_total_penalty();
        
        for (char c = 'A'; c < 'A' + comp.problem_count; c++) {
            cout << " ";
            if (team.problems.find(c) != team.problems.end()) {
                cout << team.problems.at(c).get_status_display();
            } else {
                cout << ".";
            }
        }
        cout << endl;
    }
}

void process_scroll(Competition& comp) {
    if (!comp.is_frozen) {
        cout << "[Error]Scroll failed: scoreboard has not been frozen." << endl;
        return;
    }
    
    cout << "[Info]Scroll scoreboard." << endl;
    comp.flush_scoreboard();
    print_scoreboard(comp);
    
    vector<pair<string, char>> frozen_problems;
    for (const auto& team_pair : comp.teams) {
        const string& team_name = team_pair.first;
        const Team& team = team_pair.second;
        
        for (const auto& prob_pair : team.problems) {
            char problem = prob_pair.first;
            const ProblemSubmission& prob_sub = prob_pair.second;
            
            if (prob_sub.is_frozen) {
                frozen_problems.push_back({team_name, problem});
            }
        }
    }
    
    sort(frozen_problems.begin(), frozen_problems.end(), 
         [&comp](const pair<string, char>& a, const pair<string, char>& b) {
             int rank_a = comp.get_team_ranking(a.first);
             int rank_b = comp.get_team_ranking(b.first);
             
             if (rank_a != rank_b) {
                 return rank_a > rank_b;
             }
             
             return a.second < b.second;
         });
    
    while (!frozen_problems.empty()) {
        auto current_ranking = comp.get_ranked_teams();
        
        auto it = frozen_problems.begin();
        while (it != frozen_problems.end()) {
            const string& team_name = it->first;
            char problem = it->second;
            
            int team_rank = comp.get_team_ranking(team_name);
            bool is_lowest_with_frozen = true;
            
            for (const auto& team_pair : current_ranking) {
                if (team_pair.first == team_name) break;
                
                const Team& other_team = team_pair.second;
                bool has_frozen = false;
                for (const auto& prob_pair : other_team.problems) {
                    if (prob_pair.second.is_frozen) {
                        has_frozen = true;
                        break;
                    }
                }
                if (has_frozen) {
                    is_lowest_with_frozen = false;
                    break;
                }
            }
            
            if (is_lowest_with_frozen) {
                auto old_ranking = comp.get_ranked_teams();
                int old_rank = comp.get_team_ranking(team_name);
                comp.unfreeze_problem(team_name, problem);
                
                auto new_ranking = comp.get_ranked_teams();
                int new_rank = comp.get_team_ranking(team_name);
                
                if (new_rank != old_rank && new_rank < old_rank) {
                    const Team& team = comp.teams.at(team_name);
                    cout << team_name << " ";
                    
                    // Find the team that was replaced
                    for (size_t i = 0; i < old_ranking.size(); i++) {
                        if (old_ranking[i].first == team_name) {
                            // The team moved from position i to new_rank-1
                            // Find who is now at position i
                            for (size_t j = 0; j < new_ranking.size(); j++) {
                                if (j == i) {
                                    cout << new_ranking[j].first << " ";
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    
                    cout << team.get_solved_count() << " " << team.get_total_penalty() << endl;
                }
                
                it = frozen_problems.erase(it);
                break;
            } else {
                ++it;
            }
        }
        
        if (frozen_problems.empty()) break;
        
        current_ranking = comp.get_ranked_teams();
    }
    
    comp.is_frozen = false;
    print_scoreboard(comp);
}

int main() {
    Competition competition;
    string line;
    
    while (getline(cin, line)) {
        if (line.empty()) continue;
        
        istringstream iss(line);
        string command;
        iss >> command;
        
        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            
            if (competition.add_team(team_name)) {
                cout << "[Info]Add successfully." << endl;
            } else if (competition.started) {
                cout << "[Error]Add failed: competition has started." << endl;
            } else {
                cout << "[Error]Add failed: duplicated team name." << endl;
            }
            
        } else if (command == "START") {
            string duration_str, problem_str;
            int duration, problem_count;
            iss >> duration_str >> duration >> problem_str >> problem_count;
            
            if (competition.start_competition(duration, problem_count)) {
                cout << "[Info]Competition starts." << endl;
            } else {
                cout << "[Error]Start failed: competition has started." << endl;
            }
            
        } else if (command == "SUBMIT") {
            string problem_name, by_str, team_name, with_str, status_str, at_str;
            int time;
            iss >> problem_name >> by_str >> team_name >> with_str >> status_str >> at_str >> time;
            
            competition.add_submission(problem_name, team_name, parse_status(status_str), time);
            
        } else if (command == "FLUSH") {
            competition.flush_scoreboard();
            cout << "[Info]Flush scoreboard." << endl;
            
        } else if (command == "FREEZE") {
            if (competition.freeze_scoreboard()) {
                cout << "[Info]Freeze scoreboard." << endl;
            } else {
                cout << "[Error]Freeze failed: scoreboard has been frozen." << endl;
            }
            
        } else if (command == "SCROLL") {
            process_scroll(competition);
            
        } else if (command == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            
            if (competition.teams.find(team_name) == competition.teams.end()) {
                cout << "[Error]Query ranking failed: cannot find the team." << endl;
            } else {
                cout << "[Info]Complete query ranking." << endl;
                if (competition.is_frozen) {
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled." << endl;
                }
                int ranking = competition.get_team_ranking(team_name);
                cout << "[" << team_name << "] NOW AT RANKING " << ranking << endl;
            }
            
        } else if (command == "QUERY_SUBMISSION") {
            string team_name, where_str, problem_str, and_str, status_str;
            iss >> team_name >> where_str >> problem_str >> and_str >> status_str;
            
            if (competition.teams.find(team_name) == competition.teams.end()) {
                cout << "[Error]Query submission failed: cannot find the team." << endl;
            } else {
                cout << "[Info]Complete query submission." << endl;
                
                string problem = problem_str.substr(8);
                string status = status_str.substr(7);
                
                Submission* sub = competition.find_last_submission(team_name, problem, status);
                if (sub) {
                    cout << "[" << sub->team_name << "] [" << sub->problem_name << "] [" 
                         << status_to_string(sub->status) << "] [" << sub->time << "]" << endl;
                } else {
                    cout << "Cannot find any submission." << endl;
                }
            }
            
        } else if (command == "END") {
            cout << "[Info]Competition ends." << endl;
            break;
        }
    }
    
    return 0;
}
