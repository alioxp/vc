#include "stdafx.h"
#include "pasv_port_randomizer.h"
#include "Options.h"

#include <random>

PasvPortRandomizer::PasvPortRandomizer(PasvPortManager & manager, std::wstring const& peerIP, COptions& options)
	: peerIP_(peerIP)
	, manager_(manager)
{
	if (options.GetOptionVal(OPTION_USECUSTOMPASVPORT) != 0) {
		min_ = static_cast<unsigned int>(options.GetOptionVal(OPTION_CUSTOMPASVMINPORT));
		max_ = static_cast<unsigned int>(options.GetOptionVal(OPTION_CUSTOMPASVMAXPORT));
		if (min_ > max_) {
			std::swap(min_, max_);
		}
		if (!min_) {
			min_ = 1;
		}
		else if (min_ > 65535) {
			min_ = 65535;
		}
		if (!max_) {
			max_ = 1;
		}
		else if (max_ > 65535) {
			max_ = 65535;
		}
	}
	else {
		min_ = 49152;
		max_ = 65534; // One short on purpose, I have a hunch that many firewalls and NAT routers won't like 0xFFFF
	}

	// Start with a random port in the range
	std::random_device rd;
	first_port_ = std::uniform_int_distribution<unsigned int>(min_, max_)(rd);
}

PortLease PasvPortRandomizer::GetPort()
{
	return PortLease(DoGetPort(), peerIP_, manager_);
}

unsigned int PasvPortRandomizer::DoGetPort()
{
	uint64_t const now = GetTickCount64();

	simple_lock l(manager_.mutex_);

	while (true) {
		if (!prev_port_) {
			prev_port_ = first_port_;
		}
		else {
			++prev_port_;
			if (prev_port_ < min_ || prev_port_ > max_) {
				prev_port_ = min_;
			}
			if (prev_port_ == first_port_) {
				// Wraparaound, relax requirements
				if (!allow_reuse_other_) {
					// Should not be a problem other than when using server-to-server transfers
					allow_reuse_other_ = true;
				}
				else if (!allow_reuse_same_) {
					// This can be problematic in case of peer port is the same due to the socket pair's TIME_WAIT state.
					allow_reuse_same_ = true;
				}
				else {
					// Give up
					return 0;
				}
			}
		}

		if (!manager_.connecting_[prev_port_]) {
			manager_.Prune(prev_port_, now);

			auto& es = manager_.entries_[prev_port_];
			auto it = std::find_if(es.begin(), es.end(), [&](PasvPortManager::entry const& e){ return e.peer_ == peerIP_; });
			if (it != es.end()) {
				if (allow_reuse_same_) {
					++it->leases_;
					it->expiry_ = now + 1000 * 60 * 4; // 4 minute TIME_WAIT
					manager_.connecting_[prev_port_] = true;
					return prev_port_;
				}
			}
			else if (es.empty() || allow_reuse_other_) {
				PasvPortManager::entry e;
				e.leases_ = 1;
				e.expiry_ = now + 1000 * 60 * 4; // 4 minute TIME_WAIT
				e.peer_ = peerIP_;
				es.push_back(e);
				manager_.connecting_[prev_port_] = true;
				return prev_port_;
			}
		}
	}

	return 0;
}


void PasvPortManager::Release(unsigned int p, std::wstring const& peer, bool connected)
{
	if (p && p < 65536) {
		simple_lock l(mutex_);
		
		auto& es = entries_[p];
		auto it = std::find_if(es.begin(), es.end(), [&](entry const& e){ return e.peer_ == peer; });
		if (it != es.end() && it->leases_) {
			--it->leases_;
			it->expiry_ = GetTickCount64() + 1000 * 60 * 4; // 4 minute TIME_WAIT
		}

		if (!connected) {
			connecting_[p] = false;
		}
	}
}


void PasvPortManager::SetConnected(unsigned int p, std::wstring const& peer)
{
	if (p && p < 65536) {
		simple_lock l(mutex_);
		connecting_[p] = false;
	}
}


void PasvPortManager::Prune(unsigned int port, uint64_t const now)
{
	entries_[port].erase(
			std::remove_if(entries_[port].begin(), entries_[port].end(), 
			[&](entry const& e) {
				return !e.leases_ && e.expiry_ < now;
			}),
		entries_[port].end());
}


PortLease::PortLease(PortLease && lease)
	: port_(lease.port_)
	, peerIP_(std::move(lease.peerIP_))
	, portManager_(lease.portManager_)
	, connected_(lease.connected_)
{
	lease.port_ = 0;
}

PortLease& PortLease::operator=(PortLease && lease)
{
	portManager_->Release(port_, peerIP_, connected_);

	port_ = lease.port_;
	peerIP_ = std::move(lease.peerIP_);
	portManager_ = lease.portManager_;
	connected_ = lease.connected_;
	lease.port_ = 0;
	return *this;
}

PortLease::~PortLease()
{
	portManager_->Release(port_, peerIP_, connected_);
}

PortLease::PortLease(unsigned int p, std::wstring const& peer, PasvPortManager & manager)
	: port_(p)
	, peerIP_(peer)
	, portManager_(&manager)
{
}

void PortLease::SetConnected()
{
	if (!connected_) {
		connected_ = true;
		portManager_->SetConnected(port_, peerIP_);
	}
}
