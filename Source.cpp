#include <iostream>
#include <vector>
#include <queue>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <string>
#include <set>
#include <limits>
#include <sstream>

using namespace std;

// ======================= Cấu trúc dữ liệu =======================
struct TienTrinh {
    int ma = -1;
    int den = 0; // Arrival Time (AT)
    int tongXuLy = 0; // Total CPU Burst Time (BT)
    int doUuTien = 0; // Priority (P) - ĐƯỢC ĐỌC NHƯNG BỊ BỎ QUA TRONG LOGIC LẬP LỊCH

    int io_burst_point = 0; // CPU time cần thiết trước I/O (I/O point)
    int io_duration = 0; // Thời gian I/O

    int conLai = 0; // Remaining CPU Time
    int xuLyDaXong = 0; // Total CPU time đã chạy
    int io_needed = 0; // 1 nếu cần I/O, 0 nếu đã thực hiện I/O

    int batDauLanDau = -1;
    int hoanThanh = 0;
    int quayVong = 0;
    int cho = 0;
    int phanHoi = 0;

    int thoiDiemVaoRQ = -1;
    int tongThoiGianChoChiTiet = 0;
    string waitingTimeDetails;
};

// Cấu trúc cho I/O Queue (IOQ)
struct IoEvent {
    int pid_index;
    int io_finish_time;

    // Sắp xếp Min-Heap: Thời điểm kết thúc I/O sớm nhất
    bool operator>(const IoEvent& other) const {
        if (io_finish_time != other.io_finish_time) {
            return io_finish_time > other.io_finish_time;
        }
        return pid_index > other.pid_index;
    }
};

struct Gantt {
    int batDau, ketThuc;
    int pid; // -1 cho Idle
};

// CẤU TRÚC MỚI ĐỂ LƯU LỊCH SỬ I/O
struct IOslot { int batDau, ketThuc, pid; };
vector<IOslot> io_chart; // BIẾN GLOBAL MỚI

template<typename T>
void xuat(ofstream& fout, const T& val) {
    cout << val;
    fout << val;
}

// Global variables for the update function (simplified access)
vector<TienTrinh> processes;
queue<int> ready_queue;
set<int> in_ready_queue_set;
int chiSo = 0; // Pointer cho các tiến trình chưa đến

// Hàm hỗ trợ: Đưa tiến trình vào Ready Queue
void push_to_ready_queue(int p_idx, int event_time) {
    // Chỉ thêm vào RQ nếu còn CPU time và chưa có trong RQ
    if (processes[p_idx].conLai > 0 && in_ready_queue_set.find(p_idx) == in_ready_queue_set.end()) {
        processes[p_idx].thoiDiemVaoRQ = event_time;
        ready_queue.push(p_idx);
        in_ready_queue_set.insert(p_idx);
    }
}

// =================== Chương trình chính (RR FCFS VỚI I/O) ===================
int main() {
    ifstream fin("input.txt");
    ofstream fout("output.txt");

    if (!fin.is_open()) { cerr << "Loi: Khong mo duoc file input.txt.\n"; return 1; }
    if (!fout.is_open()) { cerr << "Loi: Khong mo duoc file output.txt.\n"; return 1; }

    int n;
    if (!(fin >> n) || n <= 0) { cerr << "Loi doc so tien trinh (n).\n"; return 1; }
    processes.resize(n); // Sử dụng biến global (processes)

    // ĐỌC 5 CỘT DỮ LIỆU TỪ INPUT (AT, BT, IO Point, IO Duration, Priority)
    for (int i = 0; i < n; ++i) {
        processes[i].ma = i;
        if (!(fin >> processes[i].den >> processes[i].tongXuLy >> processes[i].io_burst_point >> processes[i].io_duration >> processes[i].doUuTien)) {
            cerr << "Loi doc du lieu tien trinh P" << i + 1 << ".\n"; return 1;
        }
        processes[i].conLai = processes[i].tongXuLy;
        
        // KIỂM TRA ĐIỀU KIỆN I/O: IO point phải nằm trong (0, Total CPU time)
        if (processes[i].io_burst_point > 0 && processes[i].io_duration > 0 && processes[i].io_burst_point < processes[i].tongXuLy) {
             processes[i].io_needed = 1; 
        }
        else {
            processes[i].io_needed = 0;
        }
    }

    int quantum;
    if (!(fin >> quantum) || quantum <= 0) { cerr << "Loi doc quantum (q).\n"; return 1; }

    // Sắp xếp theo Arrival Time, sau đó theo PID (ma)
    sort(processes.begin(), processes.end(), [](const TienTrinh& a, const TienTrinh& b) {
        if (a.den != b.den) return a.den < b.den;
        return a.ma < b.ma;
        });

    // In input summary (giữ nguyên)
    xuat(fout, "So tien trinh: "); xuat(fout, n); xuat(fout, "\n");
    xuat(fout, "Quantum: "); xuat(fout, quantum); xuat(fout, "\n\n");
    xuat(fout, "===== Danh sach tien trinh (Priority IGNORED for scheduling) =====\n");
    xuat(fout, "+------------+--------------------+---------------+------------------+-------------------+------------+\n");
    xuat(fout, "| Tien trinh | Thoi diem vao RL    | Thoi gian CPU |  Thoi diem I/O    |    Thoi gian I/O   | Do uu tien |\n");
    xuat(fout, "+------------+--------------------+---------------+------------------+-------------------+------------+\n");
    for (auto& tt : processes) {
        cout << "| P" << left << setw(10) << tt.ma + 1 << "| " << setw(19) << tt.den
            << "| " << setw(14) << tt.tongXuLy << "| " << setw(17) << tt.io_burst_point
            << "| " << setw(18) << tt.io_duration << "| " << setw(11) << tt.doUuTien << "|\n";
        fout << "| P" << left << setw(10) << tt.ma + 1 << "| " << setw(19) << tt.den
            << "|  " << setw(14) << tt.tongXuLy << "| " << setw(17) << tt.io_burst_point
            << "| " << setw(18) << tt.io_duration << "| " << setw(11) << tt.doUuTien << "|\n";
    }
    xuat(fout, "+------------+--------------------+---------------+------------------+-------------------+------------+\n\n");


    priority_queue<IoEvent, vector<IoEvent>, greater<IoEvent>> io_queue;
    int thoiGian = 0;
    int daXong = 0;
    vector<Gantt> gantt;

    // Thiết lập thời gian ban đầu nếu tiến trình đầu tiên chưa đến
    if (n > 0 && processes[0].den > 0) thoiGian = processes[0].den;

    // Lambda để cập nhật tất cả Arrivals và I/O Completions cho thời điểm hiện tại
    auto event_update = [&](int current_time) {
        // Đưa Arrivals vào RQ
        while (chiSo < n && processes[chiSo].den <= current_time) {
            push_to_ready_queue(chiSo, current_time);
            chiSo++;
        }
        // Đưa I/O Completion vào RQ
        while (!io_queue.empty() && io_queue.top().io_finish_time <= current_time) {
            int p_idx = io_queue.top().pid_index;
            io_queue.pop();
            push_to_ready_queue(p_idx, current_time);
        }
        };


    while (daXong < n) {

        event_update(thoiGian); // Cập nhật các sự kiện tại thời điểm hiện tại

        // 1. Xử lý Idle / Time Jump
        if (ready_queue.empty()) {
            int next_io_completion = io_queue.empty() ? numeric_limits<int>::max() : io_queue.top().io_finish_time;
            int next_arrival = (chiSo < n) ? processes[chiSo].den : numeric_limits<int>::max();
            int next_event_time = min(next_arrival, next_io_completion);

            if (next_event_time != numeric_limits<int>::max() && next_event_time > thoiGian) {
                gantt.push_back({ thoiGian, next_event_time, -1 });
                thoiGian = next_event_time;
                continue;
            }
            else if (next_event_time == numeric_limits<int>::max()) {
                break;
            }
        }

        // 2. Thực thi CPU
        if (!ready_queue.empty()) {
            
            // event_update(thoiGian); // Bỏ comment nếu muốn cập nhật 2 lần liên tiếp (ít hiệu quả)

            int hienTai = ready_queue.front();
            ready_queue.pop();
            in_ready_queue_set.erase(hienTai);

            // Tính toán WT CHI TIẾT
            if (processes[hienTai].thoiDiemVaoRQ != -1) {
                int thoiDiemRaRQ = thoiGian;
                int khoangCho = thoiDiemRaRQ - processes[hienTai].thoiDiemVaoRQ;
                processes[hienTai].tongThoiGianChoChiTiet += khoangCho;
                stringstream ss;
                if (!processes[hienTai].waitingTimeDetails.empty()) ss << processes[hienTai].waitingTimeDetails << " + ";
                ss << "(" << thoiDiemRaRQ << "-" << processes[hienTai].thoiDiemVaoRQ << ")";
                processes[hienTai].waitingTimeDetails = ss.str();
                processes[hienTai].thoiDiemVaoRQ = -1;
            }

            int conLai_cho_IO = numeric_limits<int>::max();

            // Nếu tiến trình cần I/O và CHƯA ĐẠT điểm I/O, tính thời gian còn lại đến I/O point
            if (processes[hienTai].io_needed == 1 && processes[hienTai].xuLyDaXong < processes[hienTai].io_burst_point) {
                conLai_cho_IO = processes[hienTai].io_burst_point - processes[hienTai].xuLyDaXong;
            }

            // Thời gian chạy (min của Quantum, Remaining CPU, và Remaining until IO point)
            int chay = min({ quantum, processes[hienTai].conLai, conLai_cho_IO });
            
            // Chỉ cập nhật CPU và Gantt nếu có thời gian chạy
            if (chay > 0) {
                int batDau = thoiGian;
                thoiGian += chay;
                processes[hienTai].conLai -= chay;
                processes[hienTai].xuLyDaXong += chay;

                if (processes[hienTai].batDauLanDau == -1) {
                    processes[hienTai].batDauLanDau = batDau;
                    processes[hienTai].phanHoi = processes[hienTai].batDauLanDau - processes[hienTai].den;
                }

                gantt.push_back({ batDau, thoiGian, processes[hienTai].ma + 1 });
            }

            // 3. Xử lý trạng thái (I/O hoặc Preempt hoặc Finish)
            event_update(thoiGian); // Cập nhật lại các sự kiện đến trong thời gian vừa chạy

            if (processes[hienTai].conLai == 0) {
                // Finish
                processes[hienTai].hoanThanh = thoiGian;
                daXong++;
            }
            // KIỂM TRA: Đạt đúng I/O point VÀ còn I/O (io_needed = 1)
            else if (processes[hienTai].xuLyDaXong == processes[hienTai].io_burst_point && processes[hienTai].io_needed == 1) {
                // I/O Burst
                processes[hienTai].io_needed = 0; // Đánh dấu I/O đã được thực hiện

                // LƯU LỊCH SỬ I/O: I/O Bắt đầu tại thời điểm kết thúc CPU (thoiGian)
                io_chart.push_back({ thoiGian, thoiGian + processes[hienTai].io_duration, processes[hienTai].ma + 1 });

                // ĐẨY VÀO IO QUEUE: Tiến trình quay lại RQ tại thời điểm hoàn thành I/O
                IoEvent event = { hienTai, thoiGian + processes[hienTai].io_duration }; 
                io_queue.push(event);
            }
            else {
                // Preempted (Hết Quantum hoặc không phải I/O)
                // Phải đẩy lại RQ nếu nó chưa hoàn thành
                push_to_ready_queue(hienTai, thoiGian);
            }
        }
    }

    // =================== Xuất Kết quả ===================
    vector<Gantt> merged_gantt;
    if (!gantt.empty()) {
        merged_gantt.push_back(gantt[0]);
        for (size_t i = 1; i < gantt.size(); ++i) {
            // Hợp nhất các đoạn CPU liền kề của cùng một tiến trình hoặc các đoạn Idle
            if (gantt[i].pid == merged_gantt.back().pid) merged_gantt.back().ketThuc = gantt[i].ketThuc;
            else merged_gantt.push_back(gantt[i]);
        }
    }

    xuat(fout, "===== Bieu do Gantt (CPU Time) =====\n");
    // HẰNG SỐ ĐỘNG DỰA TRÊN CODE CỦA BẠN ĐỂ CĂN CHỈNH
    const int TIME_SCALE_FACTOR = 4;
    const int MIN_LABEL_PAD = 2; // Khoảng đệm tối thiểu 2 ký tự hai bên nhãn

    // Dòng nhãn tiến trình (Dòng trên)
    for (size_t i = 0; i < merged_gantt.size(); ++i) {
        auto& g = merged_gantt[i];
        int doDai = g.ketThuc - g.batDau;
        if (doDai <= 0) continue;

        string label;
        if (g.pid == -1) {
            label = "Idle";
        }
        else {
            label = "P" + to_string(g.pid);
        }

        int totalWidth = max(doDai * TIME_SCALE_FACTOR - 1, (int)label.length() + MIN_LABEL_PAD * 2);
        int padding = max(0, totalWidth - (int)label.length());
        int paddingLeft = padding / 2;
        int paddingRight = padding - paddingLeft;

        cout << "|" << string(paddingLeft, ' ') << label << string(paddingRight, ' ');
        fout << "|" << string(paddingLeft, ' ') << label << string(paddingRight, ' ');
    }
    xuat(fout, "|\n"); 

    // Dòng gạch ngang (Giữ nguyên)
    for (size_t i = 0; i < merged_gantt.size(); ++i) {
        auto& g = merged_gantt[i];
        int doDai = g.ketThuc - g.batDau;
        if (doDai <= 0) continue;

        string label;
        if (g.pid == -1) label = "Idle"; else label = "P" + to_string(g.pid);

        int totalWidth = max(doDai * TIME_SCALE_FACTOR - 1, (int)label.length() + MIN_LABEL_PAD * 2);

        cout << "|" << string(totalWidth, '-');
        fout << "|" << string(totalWidth, '-');
    }
    xuat(fout, "|\n");

    // Dòng thời gian (Giữ nguyên logic căn chỉnh phức tạp)
    if (!merged_gantt.empty()) {
        string s_batDau_0 = to_string(merged_gantt[0].batDau);
        cout << s_batDau_0; fout << s_batDau_0;

        int current_pos = s_batDau_0.length();

        for (size_t i = 0; i < merged_gantt.size(); ++i) {
            auto& g = merged_gantt[i];
            int doDai = g.ketThuc - g.batDau;
            if (doDai <= 0) continue;

            string label;
            if (g.pid == -1) label = "Idle"; else label = "P" + to_string(g.pid);

            int target_pos = 0;
            for (size_t j = 0; j <= i; ++j) {
                string current_label;
                if (merged_gantt[j].pid == -1) current_label = "Idle"; else current_label = "P" + to_string(merged_gantt[j].pid);
                int current_doDai = merged_gantt[j].ketThuc - merged_gantt[j].batDau;
                int current_totalWidth = max(current_doDai * TIME_SCALE_FACTOR - 1, (int)current_label.length() + MIN_LABEL_PAD * 2);
                target_pos += 1 + current_totalWidth;
            }

            string s_ketThuc = to_string(g.ketThuc);
            int s_ketThuc_len = s_ketThuc.length();

            int spaces = target_pos - current_pos - s_ketThuc_len;

            cout << string(spaces, ' ') << s_ketThuc;
            fout << string(spaces, ' ') << s_ketThuc;

            current_pos = target_pos;
        }
    }

    // Kết thúc biểu đồ Gantt
    cout << "\n\n"; fout << "\n\n";

    // =================== Xuất I/O Chart song song CPU (CÓ KẺ BẢNG) ===================
    cout << "===== IO Chart Chi Tiet =====\n";
    fout << "===== IO Chart Chi Tiet =====\n";

    for (auto& slot : io_chart) {
        cout << "[P" << slot.pid << ": " << slot.batDau << "->" << slot.ketThuc << "] ";
        fout << "[P" << slot.pid << ": " << slot.batDau << "->" << slot.ketThuc << "] ";
    }
    cout << "\n\n"; fout << "\n\n";

    // In song song dạng time line (có kẻ bảng)
    int maxTime = 0;
    for (auto& g : merged_gantt) maxTime = max(maxTime, g.ketThuc);
    for (auto& io : io_chart) maxTime = max(maxTime, io.ketThuc);

    const int W = 4; 
    string separator(W - 1, '-'); 

    // --- Hàng thời gian ---
    cout << left << setw(10) << "Thời gian:" << "|";
    fout << left << setw(10) << "Thời gian:" << "|";
    for (int t = 0; t <= maxTime; t++) {
        cout << setw(W) << t << "|";
        fout << setw(W) << t << "|";
    }
    cout << endl; fout << endl;

    // --- Dòng kẻ ngang 1 ---
    cout << string(10, '-') << "+";
    fout << string(10, '-') << "+";
    for (int t = 0; t < maxTime + 1; t++) {
        cout << string(W, '-') << "+";
        fout << string(W, '-') << "+";
    }
    cout << endl; fout << endl;

    // --- Hàng CPU ---
    cout << left << setw(10) << "CPU:" << "|";
    fout << left << setw(10) << "CPU:" << "|";
    for (int t = 0; t < maxTime; t++) {
        int pid = -1;
        for (auto& g : merged_gantt) if (t >= g.batDau && t < g.ketThuc) pid = g.pid;

        if (pid == -1) {
            cout << setw(W) << separator << "|"; 
            fout << setw(W) << separator << "|";
        }
        else {
            cout << setw(W) << "P" + to_string(pid) << "|";
            fout << setw(W) << "P" + to_string(pid) << "|";
        }
    }
    cout << endl; fout << endl;

    // --- Dòng kẻ ngang 2 ---
    cout << string(10, '-') << "+";
    fout << string(10, '-') << "+";
    for (int t = 0; t < maxTime; t++) {
        cout << string(W, '-') << "+";
        fout << string(W, '-') << "+";
    }
    cout << endl; fout << endl;

    // --- Hàng IO ---
    cout << left << setw(10) << "I/O:" << "|";
    fout << left << setw(10) << "I/O:" << "|";
    for (int t = 0; t < maxTime; t++) {
        int pid = -1;
        for (auto& io : io_chart) if (t >= io.batDau && t < io.ketThuc) pid = io.pid;

        if (pid == -1) {
            cout << setw(W) << separator << "|"; 
            fout << setw(W) << separator << "|";
        }
        else {
            cout << setw(W) << "P" + to_string(pid) << "|";
            fout << setw(W) << "P" + to_string(pid) << "|";
        }
    }
    cout << endl; fout << endl;

    // --- Dòng kẻ ngang cuối ---
    cout << string(10, '-') << "+";
    fout << string(10, '-') << "+";
    for (int t = 0; t < maxTime; t++) {
        cout << string(W, '-') << "+";
        fout << string(W, '-') << "+";
    }
    cout << "\n\n"; fout << "\n\n";

    // =================== Xuất Bảng Kết quả Chi Tiết ===================
    double tongTAT = 0, tongWT = 0;
    sort(processes.begin(), processes.end(), [](const TienTrinh& a, const TienTrinh& b) { return a.ma < b.ma; });

    xuat(fout, "===== Bang ket qua chi tiet (Round Robin FCFS voi I/O) =====\n");
    xuat(fout, "+------------+--------------------+---------------+------------------+-------------------+------------+------------+------------+\n");
    xuat(fout, "| Tien trinh | Thoi diem vao RL    | Thoi gian CPU |  Thoi diem I/O    |    Thoi gian I/O   | Do uu tien |    Bat dau  | Hoan thanh |\n");
    xuat(fout, "+------------+--------------------+---------------+------------------+-------------------+------------+------------+------------+\n");

    const int W_PID = 12, W_DEN = 20, W_CPU = 15, W_IO_POINT = 18, W_IO_DURATION = 19, W_PRIORITY = 12, W_BD = 10, W_HOAN = 12;

    for (auto& tt : processes) {
        int TAT = tt.hoanThanh - tt.den;
        int WT = tt.tongThoiGianChoChiTiet;
        tt.quayVong = TAT;
        tt.cho = WT;
        tongTAT += TAT;
        tongWT += WT;

        cout << "| P" << left << setw(W_PID - 2) << tt.ma + 1
            << "| " << setw(W_DEN - 1) << tt.den
            << "| " << setw(W_CPU - 1) << tt.tongXuLy
            << "| " << setw(W_IO_POINT - 1) << tt.io_burst_point
            << "| " << setw(W_IO_DURATION - 1) << tt.io_duration
            << "| " << setw(W_PRIORITY - 1) << tt.doUuTien
            << "| " << setw(W_BD - 1) << tt.batDauLanDau
            << "  | " << setw(W_HOAN - 1) << tt.hoanThanh
            << "|\n";

        fout << "| P" << left << setw(W_PID - 2) << tt.ma + 1
            << "| " << setw(W_DEN - 1) << tt.den
            << "| " << setw(W_CPU - 1) << tt.tongXuLy
            << "| " << setw(W_IO_POINT - 1) << tt.io_burst_point
            << "| " << setw(W_IO_DURATION - 1) << tt.io_duration
            << "| " << setw(W_PRIORITY - 1) << tt.doUuTien
            << "| " << setw(W_BD - 1) << tt.batDauLanDau
            << "| " << setw(W_HOAN - 1) << tt.hoanThanh
            << "|\n";
    }
    xuat(fout, "+------------+--------------------+---------------+------------------+-------------------+------------+------------+------------+\n");

    xuat(fout, "\n===== Chi tiet Thoi gian Cho (Waiting Time Details) =====\n");
    for (auto& tt : processes) {
        xuat(fout, "P"); xuat(fout, tt.ma + 1);
        xuat(fout, " = ");
        if (tt.waitingTimeDetails.empty()) {
            xuat(fout, "0");
        }
        else {
            xuat(fout, tt.waitingTimeDetails);
            xuat(fout, " = "); xuat(fout, tt.tongThoiGianChoChiTiet);
        }
        xuat(fout, "\n");
    }
    xuat(fout, "\n");

    cout << fixed << setprecision(2);
    fout << fixed << setprecision(2);

    xuat(fout, "\nTong TAT = "); xuat(fout, tongTAT);
    xuat(fout, "\nTong WT (Chi tiet) = "); xuat(fout, tongWT);
    xuat(fout, "\n- TAT trung binh = "); xuat(fout, (tongTAT / n));
    xuat(fout, "\n- WT trung binh (Chi tiet) = "); xuat(fout, (tongWT / n));
    xuat(fout, "\n");

    fin.close();
    fout.close();
    return 0;
}
