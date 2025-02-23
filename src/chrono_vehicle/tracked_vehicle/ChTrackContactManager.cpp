// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Classes for monitoring contacts of tracked vehicle subsystems.
//
// =============================================================================

#include "chrono/physics/ChLoadsBody.h"

#include "chrono_vehicle/tracked_vehicle/ChTrackContactManager.h"
#include "chrono_vehicle/tracked_vehicle/ChTrackedVehicle.h"

namespace chrono {
namespace vehicle {

// -----------------------------------------------------------------------------

ChTrackContactManager::ChTrackContactManager()
    : m_initialized(false), m_flags(0), m_collect(false), m_shoe_index_L(0), m_shoe_index_R(0) {
}

void ChTrackContactManager::Process(ChTrackedVehicle* vehicle) {
    // Initialize the manager if not already done.
    if (!m_initialized) {
        m_chassis = vehicle->GetChassis();

        m_sprocket_L = vehicle->GetTrackAssembly(LEFT)->GetSprocket();
        m_sprocket_R = vehicle->GetTrackAssembly(RIGHT)->GetSprocket();

        if (vehicle->GetTrackAssembly(LEFT)->GetNumTrackShoes() > m_shoe_index_L &&
            vehicle->GetTrackAssembly(RIGHT)->GetNumTrackShoes() > m_shoe_index_R) {
            m_shoe_L = vehicle->GetTrackAssembly(LEFT)->GetTrackShoe(m_shoe_index_L);
            m_shoe_R = vehicle->GetTrackAssembly(RIGHT)->GetTrackShoe(m_shoe_index_R);
        }

        m_idler_L = vehicle->GetTrackAssembly(LEFT)->GetIdler();
        m_idler_R = vehicle->GetTrackAssembly(RIGHT)->GetIdler();

        m_initialized = true;
    }

    if (m_flags == 0)
        return;

    // Clear lists
    m_chassis_contacts.clear();
    m_sprocket_L_contacts.clear();
    m_sprocket_R_contacts.clear();
    m_shoe_L_contacts.clear();
    m_shoe_R_contacts.clear();
    m_idler_L_contacts.clear();
    m_idler_R_contacts.clear();

    // Traverse all system contacts and extract information.
    std::shared_ptr<ChTrackContactManager> shared_this(this, [](ChTrackContactManager*) {});
    vehicle->GetSystem()->GetContactContainer()->ReportAllContacts(shared_this);

    // Collect contact information data.
    // Print current time, and number of contacts involving the chassis, left/right sprockets,
    // left/right idlers, left/right track shoes, followed by the location of the contacts, in the
    // same order as above, expressed in the local frame of the respective body.
    if (m_collect) {
        // Get number of contacts in all lists;
        size_t n_chassis = m_chassis_contacts.size();
        size_t n_sprocket_L = m_sprocket_L_contacts.size();
        size_t n_sprocket_R = m_sprocket_R_contacts.size();
        size_t n_idler_L = m_idler_L_contacts.size();
        size_t n_idler_R = m_idler_R_contacts.size();
        size_t n_shoe_L = m_shoe_L_contacts.size();
        size_t n_shoe_R = m_shoe_R_contacts.size();

        // Only collect data at this time if there is at least one monitored contact
        size_t n_contacts = n_chassis + n_sprocket_L + n_sprocket_R + n_idler_L + n_idler_R + n_shoe_L + n_shoe_R;

        if (n_contacts != 0) {
            // Current simulation time
            m_csv << vehicle->GetChTime();

            // Number of contacts on vehicle parts
            m_csv << m_chassis_contacts.size();
            m_csv << m_sprocket_L_contacts.size();
            m_csv << m_sprocket_R_contacts.size();
            m_csv << m_idler_L_contacts.size();
            m_csv << m_idler_R_contacts.size();
            m_csv << m_shoe_L_contacts.size();
            m_csv << m_shoe_R_contacts.size();

            // Chassis contact points
            for (const auto& c : m_chassis_contacts) {
                m_csv << m_chassis->GetBody()->TransformPointParentToLocal(c.m_point);
            }

            for (const auto& c : m_sprocket_L_contacts) {
                m_csv << m_sprocket_L->GetGearBody()->TransformPointParentToLocal(c.m_point);            
            }

            // Right sprocket contact points
            for (const auto& c : m_sprocket_R_contacts) {
                m_csv << m_sprocket_R->GetGearBody()->TransformPointParentToLocal(c.m_point);
            }

            // Left idler contact points
            for (const auto& c : m_idler_L_contacts) {
                m_csv << m_idler_L->GetWheelBody()->TransformPointParentToLocal(c.m_point);
            }

            // Right idler contact points
            for (const auto& c : m_idler_R_contacts) {
                m_csv << m_idler_R->GetWheelBody()->TransformPointParentToLocal(c.m_point);
            }

            // Left track shoe contact points
            if (m_shoe_L) {
                for (const auto& c : m_shoe_L_contacts) {
                    m_csv << m_shoe_L->GetShoeBody()->TransformPointParentToLocal(c.m_point);
                }
            }

            // Right track shoe contact points
            if (m_shoe_R) {
                for (const auto& c : m_shoe_R_contacts) {
                    m_csv << m_shoe_R->GetShoeBody()->TransformPointParentToLocal(c.m_point);
                }
            }

            m_csv << std::endl;
        }
    }
}

// -----------------------------------------------------------------------------

bool ChTrackContactManager::InContact(TrackedCollisionFlag::Enum part) const {
    switch (part) {
        case TrackedCollisionFlag::CHASSIS:
            return m_chassis_contacts.size() != 0;
        case TrackedCollisionFlag::SPROCKET_LEFT:
            return m_sprocket_L_contacts.size() != 0;
        case TrackedCollisionFlag::SPROCKET_RIGHT:
            return m_sprocket_R_contacts.size() != 0;
        case TrackedCollisionFlag::IDLER_LEFT:
            return m_idler_L_contacts.size() != 0;
        case TrackedCollisionFlag::IDLER_RIGHT:
            return m_idler_R_contacts.size() != 0;
        case TrackedCollisionFlag::SHOES_LEFT:
            return m_shoe_L_contacts.size() != 0;
        case TrackedCollisionFlag::SHOES_RIGHT:
            return m_shoe_R_contacts.size() != 0;
        default:
            return false;
    }
}

// -----------------------------------------------------------------------------

ChVector<> ChTrackContactManager::GetSprocketResistiveTorque(VehicleSide side) const {
    const auto& contacts = (side == VehicleSide::LEFT) ? m_sprocket_L_contacts : m_sprocket_R_contacts;
    const auto& spoint =
        (side == VehicleSide::LEFT) ? m_sprocket_L->GetGearBody()->GetPos() : m_sprocket_R->GetGearBody()->GetPos();

    ChVector<> torque(0);
    for (auto& c : contacts) {
        ChVector<> F = c.m_csys * c.m_force;
        ChVector<> T = c.m_csys * c.m_torque;
        torque += (c.m_point - spoint).Cross(F) + T;
    }

    return torque;
}

// -----------------------------------------------------------------------------

bool ChTrackContactManager::OnReportContact(const ChVector<>& pA,
                                            const ChVector<>& pB,
                                            const ChMatrix33<>& plane_coord,
                                            const double& distance,
                                            const double& eff_radius,
                                            const ChVector<>& react_forces,
                                            const ChVector<>& react_torques,
                                            ChContactable* modA,
                                            ChContactable* modB) {
    ContactInfo info;

    // Ignore contacts with zero force or positive separation.
    if (distance > 0 || react_forces.IsNull())
        return true;

    // Extract contacts on chassis.
    if (IsFlagSet(TrackedCollisionFlag::CHASSIS)) {
        if (modA == m_chassis->GetBody().get()) {
            info.m_point = pA;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_chassis_contacts.push_back(info);
        }
        if (modB == m_chassis->GetBody().get()) {
            info.m_point = pB;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_chassis_contacts.push_back(info);
        }
    }

    // Extract contacts on sprockets.
    if (IsFlagSet(TrackedCollisionFlag::SPROCKET_LEFT)) {
        if (modA == m_sprocket_L->GetGearBody().get()) {
            info.m_point = pA;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_sprocket_L_contacts.push_back(info);
        }
        if (modB == m_sprocket_L->GetGearBody().get()) {
            info.m_point = pB;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_sprocket_L_contacts.push_back(info);
        }
    }

    if (IsFlagSet(TrackedCollisionFlag::SPROCKET_RIGHT)) {
        if (modA == m_sprocket_R->GetGearBody().get()) {
            info.m_point = pA;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_sprocket_R_contacts.push_back(info);
        }
        if (modB == m_sprocket_R->GetGearBody().get()) {
            info.m_point = pB;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_sprocket_R_contacts.push_back(info);
        }
    }

    // Extract contacts on track shoes (discard contacts with sprockets)
    if (IsFlagSet(TrackedCollisionFlag::SHOES_LEFT)) {
        if (modA == m_shoe_L->GetShoeBody().get() && modB != m_sprocket_L->GetGearBody().get()) {
            info.m_point = pA;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_shoe_L_contacts.push_back(info);
        }
        if (modB == m_shoe_L->GetShoeBody().get() && modA != m_sprocket_L->GetGearBody().get()) {
            info.m_point = pB;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_shoe_L_contacts.push_back(info);
        }
    }

    if (IsFlagSet(TrackedCollisionFlag::SHOES_RIGHT)) {
        if (modA == m_shoe_R->GetShoeBody().get() && modB != m_sprocket_R->GetGearBody().get()) {
            info.m_point = pA;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_shoe_R_contacts.push_back(info);
        }
        if (modB == m_shoe_R->GetShoeBody().get() && modA != m_sprocket_R->GetGearBody().get()) {
            info.m_point = pB;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_shoe_R_contacts.push_back(info);
        }
    }

    // Extract contacts on idler wheels.
    if (IsFlagSet(TrackedCollisionFlag::IDLER_LEFT)) {
        if (modA == m_idler_L->GetWheelBody().get()) {
            info.m_point = pA;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_idler_L_contacts.push_back(info);
        }
        if (modB == m_idler_L->GetWheelBody().get()) {
            info.m_point = pB;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_idler_L_contacts.push_back(info);
        }
    }

    if (IsFlagSet(TrackedCollisionFlag::IDLER_RIGHT)) {
        if (modA == m_idler_R->GetWheelBody().get()) {
            info.m_point = pA;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_idler_R_contacts.push_back(info);
        }
        if (modB == m_idler_R->GetWheelBody().get()) {
            info.m_point = pB;
            info.m_csys = plane_coord;
            info.m_force = react_forces;
            info.m_torque = react_torques;
            m_idler_R_contacts.push_back(info);
        }
    }

    // Continue scanning contacts
    return true;
}

void ChTrackContactManager::WriteContacts(const std::string& filename) {
    if (m_collect && m_flags != 0)
        m_csv.write_to_file(filename);
}

// -----------------------------------------------------------------------------

ChTrackCollisionManager::ChTrackCollisionManager(ChTrackedVehicle* vehicle) : m_idler_shoe(true), m_wheel_shoe(true) {}

void ChTrackCollisionManager::Reset() {
    // Empty collision lists
    m_collisions_idler.clear();
    m_collisions_wheel.clear();
}

static const double nrm_threshold = 0.8;

bool ChTrackCollisionManager::OnNarrowphase(collision::ChCollisionInfo& contactinfo) {
    ChBody* bodyA = dynamic_cast<ChBody*>(contactinfo.modelA->GetContactable());
    ChBody* bodyB = dynamic_cast<ChBody*>(contactinfo.modelB->GetContactable());

    if (!bodyA || !bodyB)
        return true;

    // Body B is a track shoe body
    if (bodyB->GetIdentifier() == BodyID::SHOE_BODY) {
        // Express collision normal in body A (wheel) frame
        auto nrm = bodyA->TransformDirectionParentToLocal(contactinfo.vN);

        // Identify "lateral" contacts (assumed to be with a guiding pin) and let Chrono generate contacts
        if (std::abs(nrm.y()) > nrm_threshold) {
            return true;
        }

        // Intercept and cache collisions between wheels and track pad.
        // Do not generate Chrono contact for such collisions.
        if (m_idler_shoe && bodyA->GetIdentifier() == BodyID::IDLER_BODY) {
            m_collisions_idler.push_back(contactinfo);
            return false;
        }
        if (m_wheel_shoe && bodyA->GetIdentifier() == BodyID::WHEEL_BODY) {
            m_collisions_wheel.push_back(contactinfo);
            return false;
        }
    }

    // Body A is a track shoe body
    if (bodyA->GetIdentifier() == BodyID::SHOE_BODY) {
        // Express collision normal in body B (wheel) frame
        auto nrm = bodyB->TransformDirectionParentToLocal(contactinfo.vN);

        // Identify "lateral" contacts (assumed to be with a guiding pin) and let Chrono generate contacts
        if (std::abs(nrm.y()) > nrm_threshold) {
            return true;
        }
 
        // Intercept and cache collisions between wheels and track pad.
        // Do not generate Chrono contact for such collisions.
        if (m_idler_shoe && bodyB->GetIdentifier() == BodyID::IDLER_BODY) {
            auto contactinfoS = contactinfo;
            contactinfoS.SwapModels();
            m_collisions_idler.push_back(contactinfoS);
            return false;
        }
        if (m_wheel_shoe && bodyB->GetIdentifier() == BodyID::WHEEL_BODY) {
            auto contactinfoS = contactinfo;
            contactinfoS.SwapModels();
            m_collisions_wheel.push_back(contactinfoS);
            return false;
        }
    }

    // Let Chrono generate contact for any other collision 
    return true;
}

// -----------------------------------------------------------------------------

void ChTrackCustomContact::Setup() {
    // Calculate contact forces for all current wheel-shoe collisions, calling the user-supplied callback
    ApplyForces();

    // Perform a full update of the load container 
    ChLoadContainer::Update(ChTime, false);
}

void ChTrackCustomContact::Update(double mytime, bool update_assets) {
    // Note: since Update could be called multiple times per time step, we do not invoke the
    // callback function here to calculate custom contact forces (since they are based on collision
    // detection information which only occurs once per time step). Instead, we do this in Setup.
    // We still override this function to prevent unnecessary calculations in the base class Update.
    ChTime = mytime;
}

void ChTrackCustomContact::ApplyForces() {
    // Reset the load list for this load container
    GetLoadList().clear();

    ////std::cout << "Idler-shoe collisions: " << m_collision_manager->m_collisions_idler.size() << std::endl;
    ////std::cout << "Wheel-shoe collisions: " << m_collision_manager->m_collisions_wheel.size() << std::endl;

    ChVector<> forceB;

    for (auto& cInfo : m_collision_manager->m_collisions_idler) {
        std::shared_ptr<ChBody> bodyA(static_cast<ChBody*>(cInfo.modelA->GetContactable()), [](ChBody*) {});
        std::shared_ptr<ChBody> bodyB(static_cast<ChBody*>(cInfo.modelB->GetContactable()), [](ChBody*) {});

        // Call user-provided force calculation
        ComputeForce(cInfo, bodyA, bodyB, true, forceB);

        // Apply equal and opposite forces on the two bodies (road wheel and track shoe) in contact
        auto loadA = chrono_types::make_shared<ChLoadBodyForce>(bodyA, -forceB, false, cInfo.vpA, false);
        auto loadB = chrono_types::make_shared<ChLoadBodyForce>(bodyB, +forceB, false, cInfo.vpB, false);
        Add(loadA);
        Add(loadB);
    }
    
    for (auto& cInfo : m_collision_manager->m_collisions_wheel) {
        std::shared_ptr<ChBody> bodyA(static_cast<ChBody*>(cInfo.modelA->GetContactable()), [](ChBody*) {});
        std::shared_ptr<ChBody> bodyB(static_cast<ChBody*>(cInfo.modelB->GetContactable()), [](ChBody*) {});

        // Call user-provided force calculation
        ComputeForce(cInfo, bodyA, bodyB, false, forceB);

        // Apply equal and opposite forces on the two bodies (wheel and track shoe) in contact
        auto loadA = chrono_types::make_shared<ChLoadBodyForce>(bodyA, -forceB, false, cInfo.vpA, false);
        auto loadB = chrono_types::make_shared<ChLoadBodyForce>(bodyB, +forceB, false, cInfo.vpB, false);
        Add(loadA);
        Add(loadB);
    }
}

}  // end namespace vehicle
}  // end namespace chrono
