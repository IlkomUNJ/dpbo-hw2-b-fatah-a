#include <bits/stdc++.h>
#include "utils.h"
#include "user.h"
#include "bank.h"
#include "store.h"
#include "transaction.h"

using namespace std;

// file names
static const string USERS_FILE = "users.db";
static const string ACCOUNTS_FILE = "accounts.db";
static const string ITEMS_FILE = "items.db";
static const string TX_FILE = "transactions.db";
static const string BANKTX_FILE = "banktx.db";

// in-memory data
vector<User> users;
vector<BankAccount> accounts;
vector<Item> items;
vector<Transaction> txs;
vector<BankTx> banktxs;

int next_user_id = 1, next_account_id = 1, next_item_id = 1, next_tx_id = 1, next_banktx_id = 1;

// ---- simple io helpers
vector<string> load_lines(const string &fname){
    vector<string> out;
    ifstream ifs(fname);
    if(!ifs.is_open()) return out;
    string line;
    while(getline(ifs,line)) if(!line.empty()) out.push_back(line);
    return out;
}
template<typename T>
vector<T> load_all(const string &fname, function<T(const string&)> deser){
    vector<T> out;
    for(auto &l: load_lines(fname)) out.push_back(deser(l));
    return out;
}
template<typename T>
void save_all(const string &fname, const vector<T>& v){
    ofstream ofs(fname, ios::trunc);
    for(auto &x: v) ofs << x.serialize() << "\n";
}

// ---- persistence
void load_data(){
    users = load_all<User>(USERS_FILE, [](const string &s){ return User::deserialize(s); });
    accounts = load_all<BankAccount>(ACCOUNTS_FILE, [](const string &s){ return BankAccount::deserialize(s); });
    items = load_all<Item>(ITEMS_FILE, [](const string &s){ return Item::deserialize(s); });
    txs = load_all<Transaction>(TX_FILE, [](const string &s){ return Transaction::deserialize(s); });
    banktxs = load_all<BankTx>(BANKTX_FILE, [](const string &s){ return BankTx::deserialize(s); });
    for(auto &u: users) next_user_id = max(next_user_id, u.id+1);
    for(auto &a: accounts) next_account_id = max(next_account_id, a.id+1);
    for(auto &it: items) next_item_id = max(next_item_id, it.id+1);
    for(auto &t: txs) next_tx_id = max(next_tx_id, t.id+1);
    for(auto &b: banktxs) next_banktx_id = max(next_banktx_id, b.id+1);
}
void save_data(){
    save_all<User>(USERS_FILE, users);
    save_all<BankAccount>(ACCOUNTS_FILE, accounts);
    save_all<Item>(ITEMS_FILE, items);
    save_all<Transaction>(TX_FILE, txs);
    save_all<BankTx>(BANKTX_FILE, banktxs);
}

// ---- find helpers
User* find_user_by_username(const string &u){ for(auto &x: users) if(x.username==u) return &x; return nullptr; }
User* find_user_by_id(int id){ for(auto &x: users) if(x.id==id) return &x; return nullptr; }
BankAccount* find_account_by_id(int id){ for(auto &x: accounts) if(x.id==id) return &x; return nullptr; }
Item* find_item_by_id(int id){ for(auto &x: items) if(x.id==id) return &x; return nullptr; }

// ---- bank operations
int create_account_for_user(int user_id){
    BankAccount a; a.id = next_account_id++; a.owner_user_id = user_id; a.balance = 0; a.last_activity = now_ts();
    accounts.push_back(a);
    User* u = find_user_by_id(user_id); if(u) u->account_id = a.id;
    return a.id;
}
bool topup_account(int account_id, double amount){
    BankAccount* a = find_account_by_id(account_id);
    if(!a) return false;
    a->balance += amount; a->last_activity = now_ts();
    BankTx b; b.id = next_banktx_id++; b.account_id = account_id; b.amount = amount; b.type_="topup"; b.ts = now_ts();
    banktxs.push_back(b); return true;
}
bool withdraw_account(int account_id, double amount){
    BankAccount* a = find_account_by_id(account_id);
    if(!a) return false;
    if(a->balance + 1e-9 < amount) return false;
    a->balance -= amount; a->last_activity = now_ts();
    BankTx b; b.id = next_banktx_id++; b.account_id = account_id; b.amount = -amount; b.type_="withdraw"; b.ts = now_ts();
    banktxs.push_back(b); return true;
}

// ---- transaction
int create_transaction(int buyer_id, int seller_id, int item_id, int qty){
    Item* it = find_item_by_id(item_id);
    if(!it) return -1;
    if(it->stock < qty) return -2;
    User* buyer = find_user_by_id(buyer_id); if(!buyer) return -3;
    if(buyer->account_id == -1) return -4;
    BankAccount* ba = find_account_by_id(buyer->account_id); if(!ba) return -5;
    double total = it->price * (double)qty;
    if(ba->balance + 1e-9 < total) return -6;
    ba->balance -= total; ba->last_activity = now_ts();
    BankTx b1; b1.id = next_banktx_id++; b1.account_id = ba->id; b1.amount = -total; b1.type_="payment_out"; b1.ts = now_ts(); banktxs.push_back(b1);
    User* seller = find_user_by_id(seller_id); if(!seller) return -7;
    if(seller->account_id == -1) create_account_for_user(seller->id);
    BankAccount* sa = find_account_by_id(seller->account_id);
    sa->balance += total; sa->last_activity = now_ts();
    BankTx b2; b2.id = next_banktx_id++; b2.account_id = sa->id; b2.amount = total; b2.type_="payment_in"; b2.ts = now_ts(); banktxs.push_back(b2);
    it->stock -= qty;
    Transaction t; t.id = next_tx_id++; t.buyer_id = buyer_id; t.seller_id = seller_id; t.item_id = item_id; t.qty = qty; t.amount = total; t.status="paid"; t.ts = now_ts();
    txs.push_back(t);
    return t.id;
}

bool set_tx_status(int tx_id, const string &status){
    for(auto &t: txs) if(t.id==tx_id){ t.status = status; return true; }
    return false;
}

// ---- reporting helpers
vector<Transaction> get_transactions_last_k_days_int(int k){
    vector<Transaction> out; time_t cut = days_ago(k);
    for(auto &t: txs) if(t.ts >= cut) out.push_back(t);
    sort(out.begin(), out.end(), [](const Transaction&a,const Transaction&b){ return a.ts > b.ts; });
    return out;
}
vector<Transaction> get_paid_not_completed(){
    vector<Transaction> out;
    for(auto &t: txs) if(t.status=="paid") out.push_back(t);
    return out;
}
vector<pair<int,int>> most_m_frequent_items(int m){
    unordered_map<int,int> cnt;
    for(auto &t: txs) cnt[t.item_id] += t.qty;
    vector<pair<int,int>> v(cnt.begin(), cnt.end());
    sort(v.begin(), v.end(), [](auto &a, auto &b){ return a.second > b.second; });
    if((int)v.size()>m) v.resize(m);
    return v;
}
vector<pair<int,int>> most_active_buyers_by_day(int topk){
    time_t cut = days_ago(1);
    unordered_map<int,int> cnt;
    for(auto &t: txs) if(t.ts >= cut) cnt[t.buyer_id] += 1;
    vector<pair<int,int>> v(cnt.begin(), cnt.end());
    sort(v.begin(), v.end(), [](auto &a, auto &b){ return a.second > b.second; });
    if((int)v.size()>topk) v.resize(topk);
    return v;
}
vector<pair<int,int>> most_active_sellers_by_day(int topk){
    time_t cut = days_ago(1);
    unordered_map<int,int> cnt;
    for(auto &t: txs) if(t.ts >= cut) cnt[t.seller_id] += 1;
    vector<pair<int,int>> v(cnt.begin(), cnt.end());
    sort(v.begin(), v.end(), [](auto &a, auto &b){ return a.second > b.second; });
    if((int)v.size()>topk) v.resize(topk);
    return v;
}

// bank reports
vector<BankTx> banktxs_week_backwards(){
    time_t cut = now_ts() - 7*24*3600;
    vector<BankTx> out;
    for(auto &b: banktxs) if(b.ts >= cut) out.push_back(b);
    sort(out.begin(), out.end(), [](auto &a, auto &b){ return a.ts > b.ts; });
    return out;
}
vector<BankAccount> list_all_bank_customers(){ return accounts; }
vector<BankAccount> dormant_accounts_month(){
    time_t cut = months_ago(1);
    vector<BankAccount> out;
    for(auto &a: accounts) if(a.last_activity < cut) out.push_back(a);
    return out;
}
vector<pair<int,int>> top_n_users_today_by_banktx(int n){
    time_t cut = days_ago(1);
    unordered_map<int,int> cnt;
    for(auto &b: banktxs) if(b.ts >= cut) cnt[b.account_id] += 1;
    vector<pair<int,int>> v(cnt.begin(), cnt.end());
    sort(v.begin(), v.end(), [](auto &a, auto &b){ return a.second > b.second; });
    if((int)v.size()>n) v.resize(n);
    return v;
}

// buyer reports
vector<BankTx> cashflow_for_account_interval(int account_id, time_t start, time_t end){
    vector<BankTx> out;
    for(auto &b: banktxs) if(b.account_id==account_id && b.ts>=start && b.ts<=end) out.push_back(b);
    sort(out.begin(), out.end(), [](auto &a, auto &b){ return a.ts > b.ts; });
    return out;
}
double spending_last_k_days_by_buyer(int buyer_id, int k){
    time_t cut = days_ago(k);
    double sum = 0;
    for(auto &t: txs) if(t.buyer_id==buyer_id && t.ts>=cut) sum += t.amount;
    return sum;
}

// seller analytics
vector<pair<int,int>> top_k_items_per_month_for_seller(int seller_id, int k){
    time_t cut = months_ago(1);
    unordered_map<int,int> cnt;
    for(auto &t: txs) if(t.seller_id==seller_id && t.ts>=cut) cnt[t.item_id] += t.qty;
    vector<pair<int,int>> v(cnt.begin(), cnt.end());
    sort(v.begin(), v.end(), [](auto &a, auto &b){ return a.second > b.second; });
    if((int)v.size()>k) v.resize(k);
    return v;
}
vector<pair<int,double>> loyal_customers_per_month(int seller_id, int k){
    time_t cut = months_ago(1);
    unordered_map<int,double> spent;
    for(auto &t: txs) if(t.seller_id==seller_id && t.ts>=cut) spent[t.buyer_id] += t.amount;
    vector<pair<int,double>> v(spent.begin(), spent.end());
    sort(v.begin(), v.end(), [](auto &a, auto &b){ return a.second > b.second; });
    if((int)v.size()>k) v.resize(k);
    return v;
}

// ---- CLI menus ----
void ensure_account_for_user(User* u){
    if(u->account_id==-1){
        cout << "No bank account. Creating one for you...\n";
        int aid = create_account_for_user(u->id);
        cout << "Created account id " << aid << "\n";
        save_data();
    }
}

User* do_register(){
    cout << "=== Register ===\n";
    string username, password;
    int role;
    cout << "Username: "; cin >> username;
    if(find_user_by_username(username)){ cout << "Username exists.\n"; return nullptr; }
    cout << "Password: "; cin >> password;
    cout << "Role (0 buyer,1 seller,2 admin): "; cin >> role;
    User u; u.id = next_user_id++; u.username = username; u.password = password; u.role = role; u.account_id = -1;
    users.push_back(u);
    cout << "Registered user id = " << u.id << "\n";
    save_data();
    return find_user_by_id(u.id);
}

User* do_login(){
    cout << "=== Login ===\n";
    string username, password;
    cout << "Username: "; cin >> username;
    cout << "Password: "; cin >> password;
    User* u = find_user_by_username(username);
    if(!u){ cout << "No such user\n"; return nullptr; }
    if(u->password != password){ cout << "Wrong password\n"; return nullptr; }
    cout << "Welcome " << u->username << " (" << (u->role==0? "buyer": u->role==1?"seller":"admin") << ")\n";
    return u;
}

void buyer_menu(User* me){
    ensure_account_for_user(me);
    while(true){
        cout << "\n=== BUYER MENU ===\n1.Topup\n2.Withdraw\n3.Cashflow Today\n4.Cashflow This Month\n5.Browse & Purchase\n6.List Orders\n7.Check Spending k days\n8.Logout\nChoice: ";
        int c; cin >> c;
        if(c==1){ double amt; cout<<"Amount: "; cin>>amt; if(topup_account(me->account_id, amt)){ cout<<"OK\n"; save_data(); } else cout<<"Failed\n"; }
        else if(c==2){ double amt; cout<<"Amount: "; cin>>amt; if(withdraw_account(me->account_id, amt)){ cout<<"OK\n"; save_data(); } else cout<<"Failed\n"; }
        else if(c==3){ auto v = cashflow_for_account_interval(me->account_id, days_ago(1), now_ts()); cout<<"Cashflow Today:\n"; for(auto &b:v) cout<<"ID:"<<b.id<<" amt:"<<b.amount<<" type:"<<b.type_<<" time:"<<ts_to_str(b.ts)<<"\n"; }
        else if(c==4){ auto v = cashflow_for_account_interval(me->account_id, months_ago(1), now_ts()); cout<<"Cashflow Month:\n"; for(auto &b:v) cout<<"ID:"<<b.id<<" amt:"<<b.amount<<" type:"<<b.type_<<" time:"<<ts_to_str(b.ts)<<"\n"; }
        else if(c==5){
            cout<<"Items:\n"; for(auto &it: items) cout<<"ID:"<<it.id<<" name:"<<it.name<<" price:"<<it.price<<" stock:"<<it.stock<<" seller:"<<it.seller_id<<"\n";
            int iid, qty; cout<<"ItemID (0 cancel): "; cin>>iid; if(iid==0) continue; cout<<"Qty: "; cin>>qty;
            Item* itp = find_item_by_id(iid);
            if(!itp){ cout<<"No such item\n"; continue; }
            int res = create_transaction(me->id, itp->seller_id, iid, qty);
            if(res>0){ cout<<"Purchase OK tx="<<res<<"\n"; save_data(); } else cout<<"Failed code="<<res<<"\n";
        } else if(c==6){ cout<<"Orders:\n"; for(auto &t: txs) if(t.buyer_id==me->id) cout<<"TxID:"<<t.id<<" item:"<<t.item_id<<" qty:"<<t.qty<<" amt:"<<t.amount<<" status:"<<t.status<<" time:"<<ts_to_str(t.ts)<<"\n"; }
        else if(c==7){ int k; cout<<"k days: "; cin>>k; cout<<"Spent: "<<spending_last_k_days_by_buyer(me->id,k)<<"\n"; }
        else break;
    }
}

void seller_menu(User* me){
    ensure_account_for_user(me);
    while(true){
        cout<<"\n=== SELLER MENU ===\n1.Manage Items\n2.Top k items this month\n3.Loyal customers this month\n4.View my transactions\n5.Logout\nChoice: ";
        int c; cin>>c;
        if(c==1){
            cout<<"1.Add 2.Replenish 3.Delete 4.List\nChoice: "; int a; cin>>a;
            if(a==1){ string name; double price; int stock; cout<<"Name: "; cin.ignore(); getline(cin,name); cout<<"Price: "; cin>>price; cout<<"Stock: "; cin>>stock; Item it; it.id = next_item_id++; it.seller_id = me->id; it.name=name; it.price=price; it.stock=stock; items.push_back(it); cout<<"Added id="<<it.id<<"\n"; save_data(); }
            else if(a==2){ int iid, add; cout<<"Item id: "; cin>>iid; cout<<"Add: "; cin>>add; Item* it = find_item_by_id(iid); if(!it) cout<<"No\n"; else if(it->seller_id!=me->id) cout<<"Not your item\n"; else { it->stock += add; cout<<"OK\n"; save_data(); } }
            else if(a==3){ int iid; cout<<"Item id to delete: "; cin>>iid; auto itptr = find_if(items.begin(), items.end(), [&](const Item &it){ return it.id==iid; }); if(itptr==items.end()) cout<<"No\n"; else { if(itptr->seller_id!=me->id) cout<<"Not yours\n"; else { items.erase(itptr); cout<<"Deleted\n"; save_data(); } } }
            else if(a==4){ cout<<"My items:\n"; for(auto &it: items) if(it.seller_id==me->id) cout<<"ID:"<<it.id<<" name:"<<it.name<<" price:"<<it.price<<" stock:"<<it.stock<<"\n"; }
        } else if(c==2){ int k; cout<<"k: "; cin>>k; auto v = top_k_items_per_month_for_seller(me->id,k); cout<<"Top items:\n"; for(auto &p:v){ Item* it = find_item_by_id(p.first); cout<<"ID:"<<p.first<<" name:"<< (it?it->name:"-") <<" qty:"<<p.second<<"\n"; } }
        else if(c==3){ int k; cout<<"k: "; cin>>k; auto v = loyal_customers_per_month(me->id,k); cout<<"Loyal customers:\n"; for(auto &p:v){ User* u = find_user_by_id(p.first); cout<<"User:"<<p.first<<" name:"<< (u?u->username:"-") <<" spent:"<<p.second<<"\n"; } }
        else if(c==4){ cout<<"My tx:\n"; for(auto &t: txs) if(t.seller_id==me->id) cout<<"Tx:"<<t.id<<" buyer:"<<t.buyer_id<<" item:"<<t.item_id<<" qty:"<<t.qty<<" amt:"<<t.amount<<" status:"<<t.status<<" time:"<<ts_to_str(t.ts)<<"\n"; cout<<"Change status? tx id or 0: "; int txid; cin>>txid; if(txid!=0){ cout<<"status( paid/complete/canceled ): "; string st; cin>>st; if(set_tx_status(txid,st)){ cout<<"OK\n"; save_data(); } else cout<<"Not found\n"; } }
        else break;
    }
}

void admin_menu(User* me){
    while(true){
        cout<<"\n=== ADMIN ===\n1.Tx last k days\n2.Paid not complete\n3.Most m items\n4.Most active buyers (n)\n5.Most active sellers (n)\n6.Bank reports\n7.Exit\nChoice: ";
        int c; cin>>c;
        if(c==1){ int k; cout<<"k: "; cin>>k; auto v = get_transactions_last_k_days_int(k); for(auto &t:v) cout<<"Tx:"<<t.id<<" b:"<<t.buyer_id<<" s:"<<t.seller_id<<" item:"<<t.item_id<<" amt:"<<t.amount<<" status:"<<t.status<<" time:"<<ts_to_str(t.ts)<<"\n"; }
        else if(c==2){ auto v = get_paid_not_completed(); for(auto &t:v) cout<<"Tx:"<<t.id<<" buyer:"<<t.buyer_id<<" seller:"<<t.seller_id<<" item:"<<t.item_id<<" amt:"<<t.amount<<" time:"<<ts_to_str(t.ts)<<"\n"; }
        else if(c==3){ int m; cout<<"m: "; cin>>m; auto v = most_m_frequent_items(m); for(auto &p:v){ Item* it = find_item_by_id(p.first); cout<<"Item:"<<p.first<<" name:"<< (it?it->name:"-") <<" qty:"<<p.second<<"\n"; } }
        else if(c==4){ int n; cout<<"n: "; cin>>n; auto v = most_active_buyers_by_day(n); for(auto &p:v){ User* u = find_user_by_id(p.first); cout<<"User:"<<p.first<<" name:"<< (u?u->username:"-") <<" tx:"<<p.second<<"\n"; } }
        else if(c==5){ int n; cout<<"n: "; cin>>n; auto v = most_active_sellers_by_day(n); for(auto &p:v){ User* u = find_user_by_id(p.first); cout<<"User:"<<p.first<<" name:"<< (u?u->username:"-") <<" tx:"<<p.second<<"\n"; } }
        else if(c==6){
            cout<<"Bank reports:\n1. Tx last week\n2. List customers\n3. Dormant accounts (>1 month)\n4. Top n users today\nChoice: "; int b; cin>>b;
            if(b==1){ auto v = banktxs_week_backwards(); for(auto &bt:v) cout<<"BTX:"<<bt.id<<" acct:"<<bt.account_id<<" amt:"<<bt.amount<<" type:"<<bt.type_<<" time:"<<ts_to_str(bt.ts)<<"\n"; }
            else if(b==2){ auto v = list_all_bank_customers(); for(auto &a:v) cout<<"Acct:"<<a.id<<" owner:"<<a.owner_user_id<<" bal:"<<a.balance<<" last:"<<ts_to_str(a.last_activity)<<"\n"; }
            else if(b==3){ auto v = dormant_accounts_month(); for(auto &a:v) cout<<"Acct:"<<a.id<<" owner:"<<a.owner_user_id<<" last:"<<ts_to_str(a.last_activity)<<"\n"; }
            else if(b==4){ int n; cout<<"n: "; cin>>n; auto v = top_n_users_today_by_banktx(n); for(auto &p:v) cout<<"Acct:"<<p.first<<" cnt:"<<p.second<<"\n"; }
        } else break;
    }
}

void seed_demo(){
    if(!users.empty()) return;
    User admin; admin.id = next_user_id++; admin.username="admin"; admin.password="admin"; admin.role=2; admin.account_id=-1; users.push_back(admin);
    User s1; s1.id = next_user_id++; s1.username="seller1"; s1.password="123"; s1.role=1; s1.account_id=-1; users.push_back(s1);
    User b1; b1.id = next_user_id++; b1.username="buyer1"; b1.password="123"; b1.role=0; b1.account_id=-1; users.push_back(b1);
    int aid1 = create_account_for_user(b1.id); topup_account(aid1, 1000.0);
    int aid2 = create_account_for_user(s1.id); topup_account(aid2, 100.0);
    Item it; it.id = next_item_id++; it.seller_id = s1.id; it.name="Widget A"; it.price=50; it.stock=10; items.push_back(it);
    Item it2; it2.id = next_item_id++; it2.seller_id = s1.id; it2.name="Gadget B"; it2.price=150; it2.stock=5; items.push_back(it2);
    int txid = create_transaction(b1.id, s1.id, it.id, 2);
    (void)txid;
    save_data();
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cerr << "Homework2 CLI Simulation (C++)\n";
    load_data();
    seed_demo();
    while(true){
        cout<<"\n=== MAIN MENU ===\n1.Register\n2.Login\n3.Exit\nChoice: ";
        int c; cin>>c;
        if(c==1) do_register();
        else if(c==2){
            User* u = do_login();
            if(!u) continue;
            if(u->role==0) buyer_menu(u);
            else if(u->role==1) seller_menu(u);
            else admin_menu(u);
        } else break;
    }
    cout<<"Saving...\\n"; save_data(); cout<<"Bye\n";
    return 0;
}
